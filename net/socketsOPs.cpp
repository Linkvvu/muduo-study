#include <muduo/net/socketsOPs.h>
#include <muduo/net/endian.h>
#include <muduo/base/logging.h>
#include <sys/uio.h>    // for readv and writev
#include <unistd.h>
#include <fcntl.h>
#include <cassert>

namespace {

using SA = ::sockaddr;

const SA* sockaddr_cast(const ::sockaddr_in* addr) {
    return static_cast<const SA*>(static_cast<const void*>(addr));
}

SA* sockaddr_cast(::sockaddr_in* addr) {
    return static_cast<SA*>(static_cast<void*>(addr));
}

void setNon_blockingAndCloseOnExec(int sockfd) {
    // non-block AND close on exec
    int flags = ::fcntl(sockfd, F_GETFL);
    flags |= (O_NONBLOCK | O_CLOEXEC);
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    if (ret == -1) {
        LOG_SYSFATAL << "fail to set flags(non-block AND close on exec) for file discriptor " << sockfd;
    }
}

} // namespace Global

namespace muduo {
namespace net {
namespace sockets {

int createNon_blockingOrDie() {
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNon_blockingOrDie";
    }
    return sockfd;
}

void bindOrDie(const int sockfd, const struct ::sockaddr_in* addr) {
    int ret = ::bind(sockfd, sockaddr_cast(addr), sizeof *addr);
    if (ret != 0) {
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

void listenOrDie(const int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret != 0) {
        LOG_SYSFATAL << "sockets::listenOrDie";
    }
}

int accept(const int sockfd, struct ::sockaddr_in* addr) {
    socklen_t addrlen = sizeof *addr;
    int connfd = accept4(sockfd, sockaddr_cast(addr), &addrlen, SOCK_NONBLOCK|SOCK_CLOEXEC);

    if (connfd < 0) {
        int savedErrno = errno; // 由于下文的LOG_SYSERR中可能会调用系统调用，故可能会覆盖原先的错误码
        LOG_SYSERR << "sockets::accept";
        switch (savedErrno) {
        case EAGAIN:
        case ECONNABORTED:
        case EINTR:
        case EPROTO:
        case EPERM:
        case EMFILE:
            // expected errors
            errno = savedErrno;
            break;
        case EBADF:
        case EFAULT:
        case EINVAL:
        case ENFILE:
        case ENOBUFS:
        case ENOMEM:
        case ENOTSOCK:
        case EOPNOTSUPP:
            // unexpected errors
            LOG_FATAL << "unexpected error of ::accept " << savedErrno;
            break;
        default:
            LOG_FATAL << "unknow erros of ::accept " << savedErrno;
            break;
        }
    }
    return connfd;
}

int connect(const int sockfd, const struct ::sockaddr_in* addr) {
    return ::connect(sockfd, sockaddr_cast(addr), static_cast<socklen_t>(sizeof *addr));
}

ssize_t read(const int sockfd, void* buf, std::size_t count) {
    return ::read(sockfd, buf, count);
}

ssize_t readv(const int sockfd, const struct ::iovec* iov, int count) {
    return ::readv(sockfd, iov, count);
}

ssize_t write(const int sockfd, const void* buf, std::size_t count) {
    return ::write(sockfd, buf, count);
}

ssize_t writev(const int sockefd, const struct ::iovec* iov, int count) {
    return ::writev(sockefd, iov, count);
}

void close(const int fd) {
    if (::close(fd) != 0) {
        LOG_SYSERR << "sockets::close";
    }
}

void shutdown_w(const int fd) {
    if (::shutdown(fd, SHUT_WR) != 0) {
        LOG_SYSERR << "sockets::shutdown_w";
    }
}

void to_format_ipAndPort(char* buf, std::size_t size, const struct ::sockaddr_in* addr) {
    char host[INET_ADDRSTRLEN] = "invalid";
    to_format_ip(host, INET_ADDRSTRLEN, addr);
    uint16_t port = sockets::networkToHost16(addr->sin_port);
    ::snprintf(buf, size, "%s:%u", host, port);
}

void to_format_ip(char* buf, std::size_t size, const struct ::sockaddr_in* addr) {
    assert(size >= INET_ADDRSTRLEN);
    ::inet_ntop(AF_INET, &addr->sin_addr, buf, static_cast<socklen_t>(size));
}

sockaddr_in from_ipAndPort(const char* ip, const uint16_t port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = hostToNetwork16(port);
    if (::inet_pton(AF_INET, ip, &addr.sin_addr) < 0) {
        LOG_SYSERR << "sockets::from_ipAndPort";
    }
    return addr;
}

int get_socket_error(int sockfd) {
    int optval;
    socklen_t sockLen = static_cast<socklen_t>(sizeof optval);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &sockLen) < 0) {
        return errno;
    } else {
        return optval;
    }
}

struct ::sockaddr_in get_local_addr(int sockfd) {
    struct ::sockaddr_in localAddr = {0};
    socklen_t addrLen = static_cast<socklen_t>(sizeof localAddr);
    if (::getsockname(sockfd, sockaddr_cast(&localAddr), &addrLen) < 0) {
        LOG_SYSERR << "sockets::get_local_addr";
    }
    return localAddr;
}

struct ::sockaddr_in get_peer_addr(int sockfd) {
    struct ::sockaddr_in remoteAddr = {0};
    socklen_t addrLen = static_cast<socklen_t>(sizeof remoteAddr);
    if (::getpeername(sockfd, sockaddr_cast(&remoteAddr), &addrLen) < 0) {
        LOG_SYSERR << "sockets::get_peer_addr";
    }
    return remoteAddr;
}

} // namespace sockets 
} // namespace net 
} // namespace muduo