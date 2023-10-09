#include <Socket.h>
#include <base/SocketOps.h>
using namespace muduo;

Socket::~Socket() noexcept {
    sockets::close(sockfd_);
}

