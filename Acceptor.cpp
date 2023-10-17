#include <Acceptor.h>
#include <base/SocketOps.h>
#include <Channel.h>
#include <Socket.h>
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
        std::clog << "start listening in " << GetListeningAddr().GetIpPort() << std::endl;
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
            std::cerr << "[Waring] Acceptor is not set NewConnectionCallback, "
                << "close new connection " << connfd << std::endl;
            sockets::close(connfd);
        }
    } else {
        std::cerr << "[Waring] Acceptor::HandleNewConnection occurs a error" << std::endl;
        if (errno == EMFILE) {  // Current progress opens too many open files, 占坑法
            std::cerr << "[Waring] Current progress opens too many open files" << std::endl;

            sockets::close(idleFd_);
            idleFd_ = ::accept(listener_->FileDescriptor(), nullptr, nullptr);
            sockets::close(idleFd_);
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}
