#include <muduo/Channel.h>
#include <muduo/Connector.h>
#include <muduo/base/SocketOps.h>

using namespace muduo;

const Connector::RetryDelayMs Connector::kInitRetryDelayMs(500);
const Connector::RetryDelayMs Connector::kMaxRetryDelayMs(30*1000);

Connector::Connector(EventLoop* loop, const InetAddr& server_addr)
    : loop_(loop)
    , serverAddr_(server_addr)
    { }

void Connector::Start() {
    connect_.store(true, std::memory_order::memory_order_release);
    loop_->RunInEventLoop([connector = shared_from_this()]() {
        connector->StartInLoop();
    });
}

void Connector::StartInLoop() {
    loop_->AssertInLoopThread();
    assert(opState_ == State::kDisconnected);
    if (connect_.load(std::memory_order::memory_order_acquire)) {
        DoConnect();
    } else {
        LOG_DEBUG << "give up to connect the server " << serverAddr_.GetIpPort();
    }
}

void Connector::DoConnect() {
    assert(loop_->IsInLoopThread());
    // do native connecting operation now
    int sockfd = sockets::createNonblockingOrDie(serverAddr_.GetAddressFamily());
    int ret = sockets::connect(sockfd, serverAddr_.GetNativeSockAddr());
    int savedError = ret == 0 ? 0 : errno;  // tip: the errno of standard library is TLS data
    switch (savedError) {
    case 0:             // is success
    case EINPROGRESS:   // is connecting asynchronously
    case EINTR:         // the connect operation was interrupted
    case EISCONN:       // the socket is already connected
        InitConnectOp(sockfd);
        break;
    
    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
        Retry();
        break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
        LOG_SYSERR << "connect error in Connector::DoConnect";
        sockets::close(sockfd);
        break;

    default:
        LOG_SYSERR << "unexpected error in Connector::DoConnect";
        sockets::close(sockfd);
        break;
    }
}

void Connector::InitConnectOp(int sockfd) {
    // create a channel to receive the event of async connection 
    assert(loop_->IsInLoopThread());
    assert(!chan_);
    opState_ = State::kConnecting;
#ifdef MUDUO_USE_MEMPOOL
    chan_.reset(::new Channel(loop_, sockfd));
#else
    chan_.reset(new Channel(loop_, sockfd));
#endif

    chan_->SetWriteCallback([connector = shared_from_this()]() {
        connector->HandleWrite();
    });

    chan_->SetErrorCallback([connector = shared_from_this()]() {
        connector->HandleAsyncError();
    });

    chan_->enableWriting();
}

void Connector::HandleWrite() {
    loop_->AssertInLoopThread();
    if (opState_ == State::kConnecting) {
        int sockfd = RemoveAndResetChannel();
        // tip: when an async error occurred, the events Read|Write may be activated
        int err = sockets::getSocketError(sockfd);
        if (err != 0) { // occurs a sync error
            LOG_ERROR << "Connector::HandleWrite - SO_ERROR="
                    << err << ", detail: " << strerror_thread_safe(err);
            sockets::close(sockfd);
            Retry();
        } else {    // successful to connect
            if (connect_.load(std::memory_order::memory_order_acquire)) {
                cb_(sockfd);
                opState_ = State::kConnected;
            } else {
                sockets::close(sockfd);
                opState_ = State::kDisconnected;
            }
        }
    }
}

void Connector::HandleAsyncError() {
    loop_->AssertInLoopThread();
    if (opState_ == State::kConnecting) {
        int sockfd = RemoveAndResetChannel();
        int err = sockets::getSocketError(sockfd);
        LOG_ERROR << "Connector::HandleAsyncError - SO_ERROR="
            << err << ", detail: " << strerror_thread_safe(err);
        sockets::close(sockfd);
        Retry();
    }
}

int Connector::RemoveAndResetChannel() {
    assert(loop_->IsInLoopThread());
    chan_->disableAllEvents();
    chan_->Remove();
    int sockfd = chan_->FileDescriptor();
    // warn: can't reset chan_ here, because we are inside Channel::handleEvent
    loop_->EnqueueEventLoop([connector = shared_from_this()]() {
        connector->chan_.reset();
    });
    return sockfd;
}

void Connector::Retry() {
    assert(loop_->IsInLoopThread());
    opState_ = State::kDisconnected;
    if (connect_.load(std::memory_order::memory_order_acquire)) {
        // retry
        LOG_INFO << "retry connecting to " << serverAddr_.GetIpPort()
                << " in " << retryDelayMs_.count() << "Ms";
        loop_->RunAfter(retryDelayMs_, [connector = shared_from_this()]() {
            connector->StartInLoop();
        });
        retryDelayMs_ = std::min(retryDelayMs_*2, kMaxRetryDelayMs);
    } else {
        // do nothing
        LOG_DEBUG << "give up to connect the server " << serverAddr_.GetIpPort();
    }
}

void Connector::Restart() {
    loop_->AssertInLoopThread();
    retryDelayMs_ = kInitRetryDelayMs;
    opState_ = State::kDisconnected;
    connect_.store(true, std::memory_order_release);
    StartInLoop();
}
