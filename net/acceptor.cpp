#include <muduo/net/acceptor.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/socketsOPs.h>
#include <muduo/net/inetAddress.h>
#include <fcntl.h>

using namespace muduo;
using namespace muduo::net;

acceptor::acceptor(event_loop* const loop, const inet_address& addr)
    : loop_(loop)
    , listenerSocket_(sockets::createNon_blockingOrDie())
    , listenerChannel_(loop, listenerSocket_.fd(), "listener")
    , callback_(nullptr)
    , listening_(false)
    , fd_placeholder_(::open("/dev/null", O_CLOEXEC|O_RDONLY)) {
        assert(fd_placeholder_ >= 0);
        listenerSocket_.set_reuse_Addr(true);
        listenerSocket_.bind_address(addr);
        listenerChannel_.setReadCallback(std::bind(&acceptor::handle_new_connection, this));
    }

acceptor::~acceptor() {
    if (listening_) {
        listenerChannel_.disableAllEvents();
        listenerChannel_.remove();
        sockets::close(fd_placeholder_);
    }
}

void acceptor::listen() {
    loop_->assert_loop_in_hold_thread();
    listening_ = true;
    listenerSocket_.listen();
    listenerChannel_.enableReading();
}

void acceptor::handle_new_connection() {
    loop_->assert_loop_in_hold_thread();
    inet_address peer_addr(0);

    while (true) {
        int ret = listenerSocket_.accept(&peer_addr);
        if (ret >= 0) {
            if (callback_)
                callback_(ret, peer_addr);
            else
                sockets::close(ret);
        } else {
            if (errno == EAGAIN)
                break;
            else if (errno == EINTR)
                continue;
            else if (errno == EMFILE) {
                sockets::close(fd_placeholder_);
                fd_placeholder_ = ::accept(listenerSocket_.fd(), nullptr, nullptr);
                sockets::close(fd_placeholder_);
                fd_placeholder_ = ::open("/dev/null", O_CLOEXEC|O_RDONLY);
            }
        }
    }
}
