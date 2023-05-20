#include <muduo/net/inetAddress.h>
#include <muduo/net/socketsOPs.h>
#include <muduo/net/endian.h>

using namespace muduo;
using namespace muduo::net;

inet_address::inet_address(const uint16_t port) {
    addr_.sin_family = AF_INET;
    addr_.sin_port = sockets::hostToNetwork16(port);
    addr_.sin_addr.s_addr = INADDR_ANY;
}

inet_address::inet_address(const char* ip, const uint16_t port) {
    addr_ = sockets::from_ipAndPort(ip, port);
}

string inet_address::ip() const {
    char buf[INET_ADDRSTRLEN] = {0};
    sockets::to_format_ip(buf, INET_ADDRSTRLEN, &addr_);
    return string(buf);
}

string inet_address::ipAndPort() const {
    char buf[32] = {0};
    sockets::to_format_ipAndPort(buf, sizeof buf, &addr_);
    return string(buf);
}

uint16_t inet_address::port() const  {
    return sockets::networkToHost16(addr_.sin_port);
}

