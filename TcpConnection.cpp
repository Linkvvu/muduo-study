#include <muduo/base/SocketOps.h>
#include <muduo/base/Logging.h>
#include <muduo/TcpConnection.h>
#include <muduo/EventLoop.h>
#include <muduo/Channel.h>
#include <muduo/Socket.h>
#include <new>

using namespace muduo;



TcpConnection::TcpConnection(EventLoop* owner, const std::string& name, int sockfd, const InetAddr& local_addr, const InetAddr& remote_addr)
    : loop_(owner)
    , name_(name)
    , socket_(std::make_unique<Socket>(sockfd))
    , chan_(std::make_unique<Channel>(owner, sockfd))
    , localAddr_(local_addr)
    , remoteAddr_(remote_addr)
    , inputBuffer_()
    , outputBuffer_()
{
    chan_->SetReadCallback(std::bind(&TcpConnection::HandleRead, this, std::placeholders::_1));
    chan_->SetWriteCallback(std::bind(&TcpConnection::HandleWrite, this));
    chan_->SetCloseCallback(std::bind(&TcpConnection::HandleClose, this));
    chan_->SetErrorCallback(std::bind(&TcpConnection::HandleError, this));
    LOG_DEBUG << "TcpConnection[" <<  name_ << "] is constructed at " << this << " fd=" << chan_->FileDescriptor();
}

TcpConnection::~TcpConnection() noexcept {
    LOG_DEBUG << "TcpConnection[" <<  name_ << "] is being destructed at " << this << " fd=" << chan_->FileDescriptor();
    assert(state_ == disconnected);
}

void muduo::TcpConnection::SetTcpNoDelay(bool on) {
    socket_->SetTcpNoDelay(on);
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
    State expect = connected;
    if (state_.compare_exchange_strong(expect, disconnected)) { // CAS
        chan_->disableAllEvents();
    } else if (expect == disconnecting) {   // when TcpServer is destroyed And the state of sockfd is disconnecting
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
    LOG_ERROR << "TcpConnection::HandleError[" << name_ << "] occurred a error," 
        "detail: " << strerror_thread_safe(err) << "(" << err << ")"; 
}

void TcpConnection::HandleRead(const ReceiveTimePoint_t& recv_timepoint) {
    loop_->AssertInLoopThread();
    int savedError = 0;
    ssize_t ret = inputBuffer_.ReadFd(socket_->FileDescriptor(), &savedError);
    if (ret < 0) {
        errno = savedError;
        LOG_SYSERR << "TcpConnection::HandleRead[" << name_ << "]";
        HandleError();
    } else if (ret == 0) {
        HandleClose();  // peer sends a FIN-package, so we should close the connection. (FIXME: 没有处理客户端半关闭的情况)
    } else {
        onMessageCb_(shared_from_this(), &inputBuffer_, recv_timepoint);
    }
}

void TcpConnection::HandleWrite() {
    loop_->AssertInLoopThread();
    if (chan_->IsWriting()) {
        ssize_t n = sockets::write(chan_->FileDescriptor(), outputBuffer_.Peek(), outputBuffer_.ReadableBytes());
        if (n >= 0) {
            outputBuffer_.Retrieve(n);
            if (outputBuffer_.ReadableBytes() == 0) {
                chan_->disableWriting();
                if (writeCompleteCb_) {
                    loop_->EnqueueEventLoop(std::bind(writeCompleteCb_, shared_from_this()));
                }
                if (state_ == disconnecting) {
                    ShutdownInLoop();
                }
            }
        } else {
            LOG_SYSERR << "TcpConnection::handleWrite";
            // The peer responds so slowly.
            // whether shutdown the connection directly ?
            // if (state_ == kDisconnecting)
            // {
            //   shutdownInLoop();
            // }
        }
    } else {
        LOG_TRACE << "Connection fd = " << chan_->FileDescriptor()
                << " is down, no more writing";
    }
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

void TcpConnection::Send(const char* buf, size_t len) {
    if (state_.load() == connected) {
        if (loop_->IsInLoopThread()) {
            SendInLoop(buf, len);
        } else {
            // saved buf`s data, Prevent buf from being destroyed
            std::string savedData(buf, len);
            loop_->EnqueueEventLoop([savedData = std::move(savedData), guard = shared_from_this()]() {
                guard->SendInLoop(savedData.c_str(), savedData.size());
            });
        }
    }
}

void TcpConnection::SendInLoop(const char* buf, size_t len) {
    loop_->AssertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == disconnected) {
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    // if no thing in output queue, try writing directly
    if (!chan_->IsWriting() && outputBuffer_.ReadableBytes() == 0) {
        nwrote = sockets::write(chan_->FileDescriptor(), buf, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCb_) {
                loop_->EnqueueEventLoop(std::bind(writeCompleteCb_, shared_from_this()));
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_SYSERR << "TcpConnection::SendInLoop";
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= len);

    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.ReadableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterCb_)
        {
            loop_->EnqueueEventLoop(std::bind(highWaterCb_, shared_from_this(), oldLen + remaining));
        }
        outputBuffer_.Append(buf+nwrote, remaining);
        if (!chan_->IsWriting()) {
            chan_->enableWriting();
        }
    }
}
