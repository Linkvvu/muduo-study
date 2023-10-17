#include <base/SocketOps.h>
#include <TcpConnection.h>
#include <EventLoop.h>
#include <Channel.h>
#include <Socket.h>
#include <logger.h>
using namespace muduo;

TcpConnection::TcpConnection(EventLoop* owner, const std::string& name, int sockfd, const InetAddr& local_addr, const InetAddr& remote_addr)
    : loop_(owner)
    , name_(name)
    , socket_(std::make_unique<Socket>(sockfd))
    , chan_(std::make_unique<Channel>(owner, sockfd))
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