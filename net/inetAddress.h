/// This is a public header file, it must only include public header files.
/// This file encapsulates the Internet(IPv4) address
#if !defined(MUDUO_NET_INETADDRESS_H)
#define MUDUO_NET_INETADDRESS_H

#include <muduo/base/copyable.h>
#include <muduo/base/Types.h>
#include <arpa/inet.h>

namespace muduo {
namespace net {

class inet_address : private copyable {
public:
    /// Constructs an endpoint with given port number.
    /// Mostly used in TcpServer listening.
    /// ip is set to '0.0.0.0' INADDR_ANY
    explicit inet_address(const uint16_t port);

    /// Constructs an endpoint with given ip and port.
    /// @c ip should be "1.2.3.4"
    explicit inet_address(const char* ip, const uint16_t port);

    /// Constructs an endpoint with given struct @c sockaddr_in
    /// Mostly used when accepting new connections
    inet_address(const struct ::sockaddr_in& addr) : addr_(addr) {}

    /// the class is copyable
    /// default copy/assignment are okey

    const struct ::sockaddr_in& get_sock_inetAddr() const { return addr_; }
    void set_sock_inetAddr(const ::sockaddr_in& addr) { addr_ = addr; }

    /// @return 32 bit unsigned interge ip address of big endian 
    uint32_t ip_netEndian() { return addr_.sin_addr.s_addr; }

    /// @return 16 bit unsigned interge port of big endian
    uint16_t port_netEndian() { return addr_.sin_port; }

    uint16_t port() const;
    string ip() const;
    string ipAndPort() const;
    
private:
    sockaddr_in addr_;
};

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_INETADDRESS_H
