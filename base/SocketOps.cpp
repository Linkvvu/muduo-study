#include <muduo/base/SocketOps.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Endian.h>
#include <cassert>
#include <unistd.h>

/// @note The actual structure passed for the addr argument will depend on the address family of socket,
/// So, for support IPv6, should Use 'sockaddr_in6'&'sizeof(sockaddr_in6)' in related operations

using namespace muduo;

                        /* socket-APIs */
int sockets::createNonblockingOrDie(sa_family_t family) {
    int sockfd = ::socket(family, SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0) {
        LOG_SYSFATAL << "sockets::createNonblockingOrDie";
    }
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr* addr) {
    int ret = ::bind(sockfd, addr, static_cast<socklen_t>(sizeof(sockaddr_in6)));
    if (ret < 0) {
        LOG_SYSFATAL << "sockets::bindOrDie";
    }
}

int sockets::connect(int sockfd, const struct sockaddr* addr) {
    return ::connect(sockfd, addr, static_cast<socklen_t>(sizeof(sockaddr_in6)));
}

void sockets::listenOrDie(int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        LOG_SYSFATAL << "sockets::listenOrDie";
        std::abort();
    }
}

/**
 * TODO: 令InetAddr类适配相关system call参数，e.g:  convert 'const struct sockaddr_in6* addr' to 'InetAddr addr'
 * @note The exact format of the address returned  'addr'  is  
 * determined  by  the  socket's  address  family
 * @note The  returned  address  is  truncated if the buffer provided is too small;
 * in this case, 'addrlen' will return a value greater than was supplied to the call.
*/
int sockets::accept(int listener, struct sockaddr_in6* addr) {
    socklen_t len = static_cast<socklen_t>(sizeof *addr);
    int sockfd = ::accept4(listener, sockets::sockaddr_cast(addr), &len, SOCK_NONBLOCK|SOCK_CLOEXEC);
    if (sockfd < 0) {
        int savederrno = errno;
        LOG_SYSERR << "sockets::accept";
        switch (savederrno) {
        case EAGAIN:
        case EINTR:
        case ECONNABORTED:  // 连接中断
        case EPROTO:        // 协议不匹配
        case EPERM:         // e.g：连接被防火墙拦截
        case EMFILE:        // 超过 per-process limit of open file-descriptor count
            errno = savederrno;
            break;
        case EBADF:
        case ENAVAIL:
        case ENFILE:        // OS已经没有可供打开的文件
        case ENOBUFS:
        case ENOMEM:
            LOG_FATAL << "sockets::accept unexpected error, detail: " << strerror_thread_safe(savederrno);
        default:
            LOG_FATAL << "sockets::accept unknow error, detail: " << strerror_thread_safe(savederrno);
        }
    }
    return sockfd;
}

ssize_t sockets::read(int sockfd, void* buf, size_t size) {
    return ::read(sockfd, buf, size);
}

ssize_t sockets::write(int sockfd, const void* buf, size_t size) {
    return ::write(sockfd, buf, size);
}

void sockets::close(int sockfd) {
    int ret = ::close(sockfd);
    if (ret < 0) {
        LOG_SYSERR << "sockets::close";
    }
}

void sockets::shutdownWrite(int sockfd) {
    int ret = ::shutdown(sockfd, SHUT_WR);
    if (ret < 0) {
        LOG_SYSERR << "sockets::shutdownWrite";
    }
}

int sockets::getSocketError(int sockfd) {
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        LOG_SYSERR << "sockets::getSocketError";
        return errno;
    } else {
        return optval;
    }
}


                    /* addr convert helpers */
const struct sockaddr* sockets::address::sockaddr_cast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

struct sockaddr* sockets::address::sockaddr_cast(struct sockaddr_in* addr) {
    return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

const struct sockaddr* sockets::address::sockaddr_cast(const struct sockaddr_in6* addr) {
    return static_cast<const struct sockaddr*>(static_cast<const void*>(addr));
}

struct sockaddr* sockets::address::sockaddr_cast(struct sockaddr_in6* addr) {
    return static_cast<struct sockaddr*>(static_cast<void*>(addr));
}

const struct sockaddr_in* sockets::address::convert_to_sockaddr_in(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in*>(static_cast<const void*>(addr));
}

const struct sockaddr_in6* sockets::address::convert_to_sockaddr_in6(const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in6*>(static_cast<const void*>(addr));
}

void sockets::address::toIp(char* buf, size_t size, const struct sockaddr* addr) {
    if (addr->sa_family == AF_INET) {
        assert(size >= INET_ADDRSTRLEN);
        const struct sockaddr_in* addr_inet = sockets::convert_to_sockaddr_in(addr);
        ::inet_ntop(AF_INET, &addr_inet->sin_addr, buf, static_cast<socklen_t>(size));
    } else if (addr->sa_family == AF_INET6) {
        assert(size >= INET6_ADDRSTRLEN);
        const struct sockaddr_in6* addr_inet6 = sockets::convert_to_sockaddr_in6(addr);
        ::inet_ntop(AF_INET6, &addr_inet6->sin6_addr, buf, static_cast<socklen_t>(size));
    }
}

void sockets::address::toIpPort(char* buf, size_t size, const struct sockaddr* addr) {
    if (addr->sa_family == AF_INET) {
        sockets::toIp(buf, size, addr);
        const struct sockaddr_in* addr_inet = sockets::convert_to_sockaddr_in(addr);
        uint16_t port = base::endian::BigToNative(addr_inet->sin_port);
        size_t len = std::strlen(buf);
        assert(size > len);
        std::snprintf(buf+len, size-len, ":%u", port);
    } else if (addr->sa_family == AF_INET6) {
        buf[0] = '[';
        sockets::toIp(buf, size, addr);
        const struct sockaddr_in6* addr_inet6 = sockets::convert_to_sockaddr_in6(addr);
        uint16_t port = base::endian::BigToNative(addr_inet6->sin6_port);
        size_t len = std::strlen(buf);
        assert(size > len);
        std::snprintf(buf+len, size-len, "]:%u", port);
    }
}

void sockets::address::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr) {
    assert(addr != nullptr);
    addr->sin_family = AF_INET;
    addr->sin_port = base::endian::NativeToBig(port);
    int ret = ::inet_pton(AF_INET, ip, &addr->sin_addr);
    if (ret < 0) {
        LOG_SYSERR << "sockets::fromIpPort";
    } else if (ret == 0) {
        LOG_WARN << "invalid character string for ip(" << ip << ")";
    } else {
        assert(ret == 0);   // Represent success
    }
}

void sockets::address::fromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr) {
    assert(addr != nullptr);
    addr->sin6_family = AF_INET6;
    addr->sin6_port = base::endian::NativeToBig(port);
    int ret = ::inet_pton(AF_INET6, ip, &addr->sin6_addr);
    if (ret < 0) {
        LOG_SYSERR << "sockets::fromIpPort";
    } else if (ret == 0) {
        LOG_WARN << "invalid character string for ip(" << ip << ")";
    } else {
        assert(ret == 0);   // Represent success
    }
}

union sockets::SockAddr sockets::address::getLocalAddr(int sockfd) {
    address::SockAddr result;
    struct sockaddr* exact_structure = sockaddr_cast(&result.operator sockaddr_in6 &());
    socklen_t len = sizeof(address::SockAddr);
    ::getsockname(sockfd, exact_structure, &len);
    return result;
}

union sockets::SockAddr sockets::address::getRemoteAddr(int sockfd) {
    address::SockAddr result;
    struct sockaddr* exact_structure = sockaddr_cast(&result.operator sockaddr_in6 &());
    socklen_t len = sizeof(address::SockAddr);
    ::getpeername(sockfd, exact_structure, &len);
    return result;
}

ssize_t sockets::readv(int sockfd, const struct iovec *iov, int iovcnt) {
    return ::readv(sockfd, iov, iovcnt);
}