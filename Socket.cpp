#include <muduo/Socket.h>
#include <muduo/base/SocketOps.h>
#include <muduo/base/Logging.h>
#include <netinet/tcp.h>

using namespace muduo;

Socket::~Socket() noexcept {
    sockets::close(sockfd_);
}

void Socket::Listen() {
    sockets::listenOrDie(sockfd_);
}

void Socket::BindInetAddr(const InetAddr& addr) {
    sockets::bindOrDie(sockfd_, addr.GetNativeSockAddr());
}

void Socket::SetReusePort(bool on) {
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        LOG_SYSERR << "Socket::SetReusePort"; 
    }
}

void Socket::SetReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        LOG_SYSERR << "Socket::SetReusePort"; 
    }
}

void muduo::Socket::SetTcpNoDelay(bool on) {
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        LOG_SYSERR << "Socket::SetTcpNoDelay"; 
    }
}

int Socket::Accept(InetAddr* addr) {
    sockets::SockAddr sock_addr;
    std::memset(&sock_addr, 0, sizeof sock_addr);
    int sock_fd = sockets::accept(sockfd_, &sock_addr.operator sockaddr_in6 &());
    if (sock_fd >= 0) {
        addr->SetSockAddr(sock_addr);
    }
    return sock_fd;
}

void Socket::ShutdownWrite() {
    sockets::shutdownWrite(sockfd_);
}
