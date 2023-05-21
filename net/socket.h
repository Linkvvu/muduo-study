/// This is a internal header file, you should not include this.
/// This file encapsulates socket file descriptor
#if !defined(MUDUO_NET_SOCKET_H)
#define MUDUO_NET_SOCKET_H

#include <boost/noncopyable.hpp>

namespace muduo {
namespace net {

class inet_address; // forward declaration

/// @brief use RAII for file descriptor
class socket : private boost::noncopyable {
public:
    explicit socket(const int fd) : fd_(fd) {}

    /// close the owned file descriptor
    ~socket();

    /// abort if address in use or other error
    void bind_address(const inet_address& localAddr);
    /// abort if address in use or other error
    void listen();

    /// On success, returns a non-negative integer that is
    /// a descriptor for the accepted socket, which has been
    /// set to non-blocking and close-on-exec. *peeraddr is also assigned.
    /// On error, -1 is returned, and *peeraddr is untouched.
    /// If an unexpected error Or fatal error occurs, the program abort
    int accept(inet_address* peerAddr);

    void shutdownWrite();

    ///
    /// Enable/disable TCP_NODELAY option (disable/enable Nagle's algorithm).
    ///
    /// Nagle算法的作用是减少小包的数量
    /// 怎样算小包：小于 MSS的都可以定义为小包。
    /// 如果前一个TCP段发送出去后，还没有收到对端的ACK包，那么接下来的发送数据会先累积起来不发。
    /// 等到对端返回ACK，或者数据累积已快达到MSS，才会发送出去。
    /// 禁用Nagle算法，可以避免连续发包出现延迟，这对于编写低延迟的网络服务很重要
    void set_tcp_noDelay(const bool on);

    ///
    /// Enable/disable SO_REUSEADDR option
    ///
    void set_reuse_Addr(const bool on);

    /// 
    /// Enable/disable SO_KEEPALIVE option
    /// 
    void set_tcp_keepAlive(const bool on);

    int fd() { return fd_; }

private:
    const int fd_;
};

} // namespace net 
} // namespace muduo 


#endif // MUDUO_NET_SOCKET_H
