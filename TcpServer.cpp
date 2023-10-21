#include <base/SocketOps.h>
#include <TcpConnection.h>
#include <TcpServer.h>
#include <EventLoop.h>
#include <Acceptor.h>
#include <InetAddr.h>

using namespace muduo;

TcpServer::TcpServer(EventLoop* loop, const InetAddr& addr)
    : loop_(loop)
    , addr_(std::make_unique<InetAddr>(addr))
    , acceptor_(std::make_unique<Acceptor>(loop, addr, true))   // FIXME: set "option reuse-port" by evnironment-variable  
    , conns_()
{
    acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::HandleNewConnection, this,
        std::placeholders::_1, std::placeholders::_2));    
}

TcpServer::~TcpServer() noexcept {

}

std::string TcpServer::GetIp() const {
    return acceptor_->GetIp();
}

std::string TcpServer::GetIpPort() const {
    return acceptor_->GetIpPort();
}

void TcpServer::HandleNewConnection(int connfd, const InetAddr& remote_addr) {
    loop_->AssertInLoopThread();
    std::string new_conn_name = /*Server-name#*/ remote_addr.GetIpPort() + "@" + std::to_string(nextConnID_++); 
    InetAddr local_addr(sockets::getLocalAddr(connfd));
    TcpConnectionPtr new_conn_ptr = TcpConnection::CreateTcpConnection(loop_, std::move(new_conn_name), connfd, local_addr, remote_addr);
    std::clog << "TcpServer::HandleNewConnection: new connection [" << new_conn_name << "] from " << remote_addr.GetIpPort() << std::endl;
    conns_[new_conn_name] = new_conn_ptr;   // add current connection to list
    new_conn_ptr->SetConnectionCallback(connectionCb_);
    new_conn_ptr->SetOnMessageCallback(messageCb_);
    new_conn_ptr->SetOnCloseCallback(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
    new_conn_ptr->StepIntoEstablished();
}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn) {
    loop_->AssertInLoopThread();
    assert(conn.use_count() > 1);
    int ret = conns_.erase(conn->GetName());
    assert(ret == 1);
    conn->StepIntoDestroyed();
}

void TcpServer::ListenAndServe() {
    bool expected = false;
    if (serving_.compare_exchange_strong(expected, true)) { // CAS
        loop_->RunInEventLoop([this]() {
            this->loop_->RunInEventLoop(
                std::bind(&Acceptor::Listen, this->acceptor_.get())
            );
        });
    }
}

