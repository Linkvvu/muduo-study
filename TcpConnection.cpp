#include <base/SocketOps.h>
#include <base/MemPool.h>
#include <new>
#include <TcpConnection.h>
#include <EventLoop.h>
#include <Channel.h>
#include <Socket.h>
#include <logger.h>
using namespace muduo;

/// invoke T`s constructor with placement-new on 'position'
template <typename T, typename... Args>
T* NewInstanceWithPlacement(void* position, Args... args) {
    return new (position) T(args...);
}

namespace muduo::base {
///  destroy and free resource who alloc from mem-pool
template<typename T>
void PdeleterWithPool(MemPool* pool, T* ptr) {
    ptr->~T();
    pool->Pfree(ptr);
}
} // namespace muduo::base 

TcpConnectionPtr muduo::TcpConnection::CreateTcpConnection(EventLoop* owner, const std::string& name, int sockfd, const InetAddr& local_addr, const InetAddr& remote_addr) {
    /* create a connection-level memory pool */
    base::MemPool* p = base::MemPool::CreateMemoryPool(1024);
    /* construct a connection instance */
    void* m = p->Palloc(sizeof (TcpConnection));
    TcpConnection* conn = new (m) TcpConnection(owner, p, name, sockfd, local_addr, remote_addr);
    // assert(conn == p);
    /* wrap the connection with std::shared_ptr */
    return std::shared_ptr<TcpConnection>(conn, &TcpConnection::DestroyTcpConnection);
}

void muduo::TcpConnection::DestroyTcpConnection(TcpConnection* conn) {
    /// pool own most of resoucres of the connection,
    /// So life cycle of Mem-Pool must be longer than that of connection instance
    
    /* save mem-pool address */
    base::MemPool* connection_MemPool = conn->pool_;
    /* destroy tcp-connection instance */
    conn->~TcpConnection();
    /* free connection-level memory pool */
    base::MemPool::DestroyMemoryPool(connection_MemPool);
}

TcpConnection::TcpConnection(EventLoop* owner, base::MemPool* pool, const std::string& name, int sockfd, const InetAddr& local_addr, const InetAddr& remote_addr)
    : loop_(owner)
    , pool_(pool)
    , name_(name)
    , socket_(NewInstanceWithPlacement<Socket>(pool_->Palloc(sizeof(Socket)), sockfd), 
        [this](Socket* ptr) { base::PdeleterWithPool(pool_, ptr);})
    , chan_(NewInstanceWithPlacement<Channel>(pool_->Palloc(sizeof(Channel)), owner, sockfd), 
        std::bind(&base::PdeleterWithPool<Channel>, pool_, std::placeholders::_1))
    , localAddr_(local_addr)
    , remoteAddr_(remote_addr)
{
    chan_->SetReadCallback(std::bind(&TcpConnection::HandleRead, this, std::placeholders::_1));
    chan_->SetWriteCallback(std::bind(&TcpConnection::HandleWrite, this));
    chan_->SetCloseCallback(std::bind(&TcpConnection::HandleClose, this));
    chan_->SetErrorCallback(std::bind(&TcpConnection::HandleError, this));
}

TcpConnection::~TcpConnection() noexcept {
    /// TODO: ...
    assert(state_ == disconnected);
}

void TcpConnection::StepIntoEstablished() {
    loop_->AssertInLoopThread();
    assert(state_ == connecting);
    state_.store(connected);
    chan_->Tie(shared_from_this());
    chan_->EnableReading();
    connectionCb_(shared_from_this());
}

void TcpConnection::StepIntoDestroyed() {
    loop_->AssertInLoopThread();
    assert(state_ == disconnected || state_ == connected);
    if (state_ == connected) {
        state_ = disconnected;
        chan_->disableAllEvents();
    }
    connectionCb_(shared_from_this());
    chan_->Remove();
}

void TcpConnection::HandleClose() {
    loop_->AssertInLoopThread();
    assert(state_ == connected || state_ == disconnecting);
    chan_->disableAllEvents();  // prevent poll trigger POLLOUT again
    state_ = disconnected;
    onCloseCb_(shared_from_this());
}

void TcpConnection::HandleError() {
    loop_->AssertInLoopThread();
    int err = sockets::getSocketError(socket_->FileDescriptor());
    std::cerr << "TcpConnection::HandleError [" << name_ << "] occurred a error: " << strerror_thread_safe(errno) << std::endl;
}

void TcpConnection::HandleRead(const ReceiveTimePoint_t& recv_timepoint) {
    loop_->AssertInLoopThread();
    char temp_buf[1024] {0};
    ssize_t ret = sockets::read(socket_->FileDescriptor(), temp_buf, sizeof temp_buf);
    if (ret < 0) {
        std::cerr << "TcpConnection::HandleRead [" << name_ << "] occurred a error: " << strerror_thread_safe(errno) << std::endl;
        HandleError();
    } else if (ret == 0) {
        HandleClose();  // peer sends a FIN-package, so we should close the connection. (FIXME: 没有处理客户端半关闭的情况)
    } else {
        onMessageCb_(shared_from_this(), temp_buf, ret, recv_timepoint);
    }
}

void TcpConnection::HandleWrite() {
    loop_->AssertInLoopThread();
    /// TODO: write-logic
}

void TcpConnection::Shutdown() {
    TcpConnection::State expected = connected;
    if (state_.compare_exchange_strong(expected, disconnecting)) {  // CAS
        /// FIXME: 如果TcpConnection::ShutdownInLoop被加入至pendingQueue
        /// 但在调用绑定的TcpConnection::ShutdownInLoop回调之前，当前connection的引用计数为0了(即当前连接被销毁)
        /// 那么，this指针将会成为野指针
        /// FIXME: pass 'shard_ptr' instead of 'this' when binding the callback
        loop_->RunInEventLoop(std::bind(&TcpConnection::ShutdownInLoop, this));
    }
}

void TcpConnection::ShutdownInLoop() {
    loop_->AssertInLoopThread();
    if (!chan_->IsWriting()) {
        socket_->ShutdownWrite();
    } /* else {
        This is handled by write-handler When the send-operation is complete
    } */
}