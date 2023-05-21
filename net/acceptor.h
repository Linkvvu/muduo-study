#if !defined(MUDUO_NET_ACCEPTOR_H)
#define MUDUO_NET_ACCEPTOR_H

#include <muduo/net/channel.h>
#include <muduo/net/socket.h>
#include <boost/noncopyable.hpp>
#include <functional>

namespace muduo {
namespace net {

class event_loop;   // forward declaration
class inet_address; // forward declaration

/// @brief Acceptor of incoming TCP connections.
class acceptor : private boost::noncopyable {
public:
    using newConnCallback = std::function<void(int socketFd, const inet_address& addr)>;

    acceptor(event_loop* const loop, const inet_address& addr);
    
    ~acceptor();

    void listen();

    void set_newConn_callback(const newConnCallback& func) { callback_ = func; }

    void handle_new_connection();

private:
    event_loop* const loop_;
    socket listenerSocket_;
    channel listenerChannel_;
    newConnCallback callback_;
    bool listening_;
    int fd_placeholder_;
};


} // namespace net 
} // namespace muduo 
#endif // MUDUO_NET_ACCEPTOR_H
