#if !defined(MUDUO_INETADDR_H)
#define MUDUO_INETADDR_H

#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <arpa/inet.h>
#include <cstddef>  // for offsetof
#include <string>

namespace muduo {
namespace sockets {
namespace address {

/// @brief Represents a IPv4 address OR IPv6 address
union SockAddr {
    struct sockaddr_in inet4;
    struct sockaddr_in6 inet6;

    /// FIXME: don`t use "operator type statement", it`s not graceful And clear
    explicit operator const sockaddr_in6&() const {
        return inet6;
    }
    explicit operator sockaddr_in6&() {
        return inet6;
    }
};
static_assert(sizeof(SockAddr) == sizeof(struct sockaddr_in6), 
    "size of 'union sockets::address::SockAddr' is not equal to size of 'struct sockaddr_in6'");
static_assert(offsetof(sockaddr_in, sockaddr_in::sin_family) == 0,
    "sockaddr_in::sin_family offset isn`t 0");
static_assert(offsetof(sockaddr_in6, sockaddr_in6::sin6_family) == 0,
    "sockaddr_in6::sin6_family offset isn`t 0");
static_assert(offsetof(SockAddr, SockAddr::inet4) == 0 &&
    offsetof(SockAddr, SockAddr::inet4) == offsetof(SockAddr, SockAddr::inet6),
    "'union sockets::address::SockAddr' is not supported");

} // namespace address 
} // namespace sockets 


/// @brief Represents a copyable IPv4 OR IPv6 address 
#ifdef MUDUO_USE_MEMPOOL
class InetAddr final : public base::detail::ManagedByMempoolAble<InetAddr> {
#else
class InetAddr {
#endif
    using SockAddr = sockets::address::SockAddr;
public:
    explicit InetAddr(const SockAddr& addr)
        : addr_(addr)
        { }
    
    /// Constructs an endpoint with given port number(By default, it`s determined by the OS).
    /// Mostly used in TcpServer listening.
    explicit InetAddr(uint16_t port = 0, bool loopback_only = false, bool ipv6 = false);
    explicit InetAddr(const std::string& ip, uint16_t port, bool ipv6 = false);

    // default copy-constructor & copy-assigned are okey.
    InetAddr(const InetAddr&) = default;
    InetAddr& operator=(const InetAddr&) = default;    
    // default destructor is okey.
    
    sa_family_t GetAddressFamily() const { return addr_.inet4.sin_family; }
    const struct sockaddr* GetNativeSockAddr() const;

    void SetSockAddr(const SockAddr& a)
    { addr_ = a; }
    void SetSockAddrInet4(const struct sockaddr_in& a)
    { addr_.inet4 = a; }
    void SetSockAddrInet6(const struct sockaddr_in6& a)
    { addr_.inet6 = a; }

    std::string GetIpPort() const;
    std::string GetIp() const;

private:
    SockAddr addr_;
};

static_assert(sizeof(InetAddr) == sizeof(sockets::address::SockAddr),
    "muduo::InetAddr size isn`t equal to sockets::address::SockAddr size");

} // namespace muduo 

#endif // MUDUO_INETADDR_H
