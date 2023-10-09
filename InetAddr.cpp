#include <InetAddr.h>
#include <base/SocketOps.h>
#include <base/Endian.h>
#include <cstring>
#include <cassert>

using namespace muduo;

namespace {
    static const in_addr_t kInaddrAny = INADDR_ANY;
    static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
} // namespace 

InetAddr::InetAddr(uint16_t port, bool loopback_only, bool ipv6) {
    static_assert(offsetof(InetAddr, InetAddr::addr_.inet4) == 0, "offset of InetAddr::addr_.inet4 isn`t 0");
    static_assert(offsetof(InetAddr, InetAddr::addr_.inet6) == 0, "offset of InetAddr::addr_.inet6 isn`t 0");

    if (ipv6) {
        std::memset(&addr_.inet6, 0, sizeof addr_.inet6);
        addr_.inet6.sin6_family = AF_INET6;
        addr_.inet6.sin6_port = base::endian::NativeToBig(port);
        in6_addr ip = loopback_only ? in6addr_loopback : in6addr_any;
        addr_.inet6.sin6_addr = ip;
    } else {
        std::memset(&addr_.inet4, 0, sizeof addr_.inet4);
        addr_.inet4.sin_family = AF_INET;
        addr_.inet4.sin_port = base::endian::NativeToBig(port);
        in_addr_t ip = loopback_only ? kInaddrLoopback : kInaddrAny;
        addr_.inet4.sin_addr.s_addr = base::endian::NativeToBig(ip);
    }
}

InetAddr::InetAddr(const std::string& ip, uint16_t port, bool ipv6) {
    static_assert(offsetof(InetAddr, InetAddr::addr_.inet4) == 0, "offset of InetAddr::addr_.inet4 isn`t 0");
    static_assert(offsetof(InetAddr, InetAddr::addr_.inet6) == 0, "offset of InetAddr::addr_.inet6 isn`t 0");

    if (ipv6) {
        std::memset(&addr_.inet6, 0, sizeof addr_.inet6);
        sockets::fromIpPort(ip.c_str(), port, &addr_.inet6);
    } else {
        std::memset(&addr_.inet4, 0, sizeof addr_.inet4);
        sockets::fromIpPort(ip.c_str(), port, &addr_.inet4);
    }
}

const sockaddr* InetAddr::GetNativeSockAddr() {
    const sockaddr* tmp = sockets::sockaddr_cast(&addr_.inet4);
    // const sockaddr* tmp = sockets::sockaddr_cast(&addr_.inet6);
    if (tmp->sa_family == AF_INET) {
        return tmp;
    } else {
        assert(tmp->sa_family == AF_INET6);
        return sockets::sockaddr_cast(&addr_.inet6);
    }
}

