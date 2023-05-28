#if !defined(MUDUO_NET_TCPSERVER_H)
#define MUDUO_NET_TCPSERVER_H

#include <muduo/base/Types.h>
#include <muduo/net/callBacks.h>
#include <boost/noncopyable.hpp>
#include <unordered_map>
#include <atomic>
#include <memory>
#include <cassert>

namespace muduo {
namespace net {

class event_loop;           // forward declaration
class socket;               // forward declaration
class inet_address;         // forward declaration
class acceptor;             // forward declaration
class eventLoop_threadPool; // forward declaration

class tcp_server : boost::noncopyable {
public:
    tcp_server(event_loop* const loop, const inet_address& listener_addr, const string& name);
    ~tcp_server();

    string ipAndPort() { return ipAndPort_; }
    string server_name() { return serverName_; }

    void set_onMessage_callback(const onMsgCallback_t& cb) { onMsgCb_ = cb; }
    void set_onConnection_callback(const onConnectionCallback_t& cb) { onConnCb_ = cb; }
    void set_IO_threadInit_callback(const IO_thread_initializeCallback_t& cb) { ioThreadCb_ = cb; }
    void set_IO_threadNum(int n) { assert(n >= 0); ioThreadNum_ = n; }
    /// @brief server start listen
    /// @details be invoked across threads, thread safe.
    void start();

private:
    // key: conn friendly name, val: tcp_connection_ptr
    using tcp_connection_ptr = std::shared_ptr<tcp_connection>;
    using connection_list_t = std::unordered_map<string, tcp_connection_ptr>;

    void handle_newConnection(int sockfd, const inet_address& addr);
    void remove_connection(const std::shared_ptr<tcp_connection>& conn);
private:
    event_loop* const loop_;
    string serverName_;
    string ipAndPort_;
    std::unique_ptr<acceptor> acceptor_;
    std::atomic<bool> started_;
    int ioThreadNum_;
    onMsgCallback_t onMsgCb_;
    onConnectionCallback_t onConnCb_;
    IO_thread_initializeCallback_t ioThreadCb_;
    std::unique_ptr<eventLoop_threadPool> ioThreadPool_;

    // keep thread safe
    uint32_t nextConnId_;
    connection_list_t conns_;
};

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_TCPSERVER_H
