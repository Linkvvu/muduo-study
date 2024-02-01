#include <muduo/TcpClient.h>
#include <muduo/Channel.h>
#include <muduo/Connector.h>
#include <muduo/TcpConnection.h>
#include <muduo/base/SocketOps.h>

using namespace muduo;

TcpClientPtr muduo::CreateTcpClient(EventLoop* loop, const InetAddr& server_addr, std::string name) {
    TcpClientPtr client = std::shared_ptr<TcpClient>(new TcpClient(loop, server_addr, std::move(name)));

    // saves a weak-ptr to prevent accessing the destroyed TcpClient instance
    client->connector_->SetConnectSuccessfullyCallback([weak = client->weak_from_this()](int sockfd) {
        std::shared_ptr<TcpClient> client = weak.lock();    // as guard to prevent the client instance destroying now
        if (client) {
            // the target client isn't destroyed, invoke callback
            client->HandleConnectSuccessfully(sockfd);
        } else {    
            // the target client was destroyed, close the connected native connection
            sockets::close(sockfd);
            LOG_WARN << "the client was destroyed, no longer connect";
        }
    });

    return client;
}

TcpClient::TcpClient(EventLoop* loop, const InetAddr& server_addr, std::string name)
    : loop_(loop)
    , connector_(std::make_unique<Connector>(loop_, server_addr))
    , clientName_(std::move(name))
{
    /// @bug can't invoke @c XXX_from_this in the constructor,
    ///      because underlying @c weak-ptr is initialized after constructor  

    LOG_DEBUG << "TcpClient::TcpClient[" << clientName_
            << "] - connector " << connector_.get();
}

TcpClient::~TcpClient() {
    LOG_DEBUG << "TcpClient::TcpClient[" << clientName_
            << "] - connector " << connector_.get();
    
    TcpConnectionPtr conn;
    {
        std::lock_guard<std::mutex> guard(mutex_);
        conn = connection_;
    }

    if (conn) { 
        // should invoke TcpConnection::StepIntoDestroyed to prepare for the impending destruction
        conn->GetEventLoop()->EnqueueEventLoop(
            [conn]() {
                // force close, as if read 0 bytes
                if (conn->state_ == TcpConnection::connected || conn->state_ == TcpConnection::connecting)
                    conn->HandleClose();
            }
        );
    } else {
        // no longer attempt to connect, if the client start
        this->Stop();
    }
}

void TcpClient::Connect() {
    doConnect_.store(true, std::memory_order::memory_order_release);
    connector_->Start();
}

void TcpClient::Shutdown() {
    bool expect = true;
    // checks whether did connecting
    bool isConnect = doConnect_.compare_exchange_strong(expect, false);
    if (isConnect) {
        
        {
            std::lock_guard<std::mutex> guard(mutex_);
            if (connection_) {
                connection_->Shutdown();
            }
        }
        
    }    
}

void TcpClient::Stop() {
    bool expect = true;
    // checks whether did connecting
    bool isConnect = doConnect_.compare_exchange_strong(expect, false);
    if (isConnect) {
        connector_->Stop();
    }
}

void TcpClient::HandleConnectSuccessfully(int sockfd) {
    loop_->AssertInLoopThread();
    InetAddr remote_addr(sockets::getRemoteAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", remote_addr.GetIpPort().c_str(), nextConnId_++);
    std::string conn_name = clientName_ + buf;

    InetAddr local_addr(sockets::getLocalAddr(sockfd));
    TcpConnectionPtr conn_ptr;
#ifdef MUDUO_USE_MEMPOOL
    // the TcpConnection instance be allocated from memory pool of the specified loop,
    // so it must be deallocated in the thread where the specified loop is located
    conn_ptr = std::shared_ptr<TcpConnection>(new (loop_->GetMemoryPool()) TcpConnection(loop_, conn_name, sockfd, local_addr, remote_addr),
        [loop = this->GetEventLoop()](TcpConnection* conn) {
            // destroys the connection instance
            conn->~TcpConnection();
            // thread-safely deallocate the storage
            loop->RunInEventLoop([loop, ptr = conn]() {
                loop->GetMemoryPool()->deallocate(ptr, sizeof(TcpConnection));
            });
        });
#else
    // The TcpConnection instance be allocated from heap
    conn_ptr = std::make_shared<TcpConnection>(loop_, conn_name, sockfd, local_addr, remote_addr);
#endif
    conn_ptr->SetConnectionCallback(connectionCb_);
    conn_ptr->SetOnMessageCallback(onMessageCb_);
    conn_ptr->SetWriteCompleteCallback(writeCompleteCb_);
    conn_ptr->SetOnCloseCallback([weak = weak_from_this()](const TcpConnectionPtr& conn) {
        std::shared_ptr<TcpClient> client = weak.lock();
        if (client) {
            client->HandleRemoveConnection(conn);
        } else {
            // directly destroy
            conn->StepIntoDestroyed();
        }
    });

    {
        std::lock_guard<std::mutex> guard(mutex_);
        connection_ = conn_ptr;
    }

    conn_ptr->StepIntoEstablished();
}

void TcpClient::HandleRemoveConnection(const TcpConnectionPtr& conn) {
    loop_->AssertInLoopThread();
    assert(loop_ == conn->GetEventLoop());
    {
        std::lock_guard<std::mutex> guard(mutex_);
        assert(conn == connection_);
        connection_.reset();        
    }

    // destroys the connection
    conn->StepIntoDestroyed();

    // attempt to reconnect
    if (doConnect_ && retry_) {
        LOG_INFO << "TcpClient::connect[" << clientName_ << "] - Reconnecting to "
                << connector_->GetServerAddress().GetIpPort();
        connector_->Restart();
    }
}

