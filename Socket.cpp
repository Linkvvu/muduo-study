#include <Socket.h>
#include <base/SocketOps.h>
#include <logger.h>
#include <iostream>
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
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &on, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        std::cerr << "Socket::SetReusePort: " << strerror_thread_safe(errno) << std::endl; 
    }
}

void Socket::SetReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &on, static_cast<socklen_t>(sizeof optval));
    if (ret < 0) {
        std::cerr << "Socket::SetReusePort: " << strerror_thread_safe(errno) << std::endl; 
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
