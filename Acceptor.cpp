#include <muduo/Acceptor.h>
#include <muduo/base/SocketOps.h>
#include <muduo/Channel.h>
#include <muduo/Socket.h>
#include <fcntl.h>

using namespace muduo;

Acceptor::Acceptor(EventLoop* owner, const InetAddr& addr, bool reuse_port)
    : owner_(owner)
    , addr_(addr)
    , listener_(std::make_unique<Socket>(sockets::createNonblockingOrDie(addr.GetAddressFamily())))
    , channel_(std::make_unique<Channel>(owner_, listener_->FileDescriptor()))
    , idleFd_(::open("/dev/null", O_RDONLY|O_CLOEXEC))
    , listening_(false)
{
    listener_->SetReuseAddr(true);
    listener_->SetReusePort(reuse_port);
    listener_->BindInetAddr(addr);
    channel_->SetReadCallback(std::bind(&Acceptor::HandleNewConnection, this));
}

Acceptor::~Acceptor() noexcept {
    assert(owner_->IsInLoopThread());
    if (listening_) {
        channel_->disableAllEvents();
        channel_->Remove();
    }
}

void Acceptor::Listen() {
    owner_->AssertInLoopThread();
    /* if (listening_.test_and_set() == false) ... */
    bool expect = false;
    if (listening_.compare_exchange_strong(expect, true)) { // CAS
        listener_->Listen();
        channel_->EnableReading();
    }
}

void Acceptor::HandleNewConnection() {
    owner_->AssertInLoopThread();
    InetAddr remote_addr;
    int connfd = listener_->Accept(&remote_addr);
    if (connfd >= 0) {
        if (onNewConnectionCb_) {
            onNewConnectionCb_(connfd, remote_addr);
        } else {
            LOG_WARN << "Acceptor is not set NewConnectionCallback, "
                << "close new connection(" << connfd << ")now";
            sockets::close(connfd);
        }
    } else {
        LOG_SYSERR << "in Acceptor::HandleNewConnection" ;
        if (errno == EMFILE) {  // Current progress opens too many open files, 占坑法
            LOG_WARN << "Current progress opens too many open files";

            sockets::close(idleFd_);
            idleFd_ = ::accept(listener_->FileDescriptor(), nullptr, nullptr);
            sockets::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
