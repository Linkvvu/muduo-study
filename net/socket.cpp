#include <muduo/net/socket.h>
#include <muduo/net/socketsOPs.h>
#include <muduo/net/inetAddress.h>
#include <muduo/base/logging.h>
#include <netinet/tcp.h>    // for TCP_NODELAY

using namespace muduo;
using namespace muduo::net;

socket::~socket() {
    if (sockets::close(fd_)) {
        LOG_DEBUG << "file descriptor " << fd_ << " is closed";
    }
}

void socket::bind_address(const inet_address& localAddr) {
    sockets::bindOrDie(fd_, &localAddr.get_sock_inetAddr());
}

void socket::listen() {
    sockets::listenOrDie(fd_);
}

int socket::accept(inet_address* peerAddr) {
    struct ::sockaddr_in remoteAddr;
    int fd = sockets::accept(fd_, &remoteAddr);
    if (fd >= 0) {
        peerAddr->set_sock_inetAddr(remoteAddr);
    }
    return fd;
}

void socket::shutdownWrite() { sockets::shutdown_w(fd_); }

void socket::set_tcp_noDelay(const bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval); 
}

void socket::set_reuse_Addr(const bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void socket::set_tcp_keepAlive(const bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}