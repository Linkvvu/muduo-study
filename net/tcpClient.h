/// This is a public header file, it must only include public header files.
#if !defined(MUDUO_NET_TCPCLIENT_H)
#define MUDUO_NET_TCPCLIENT_H

#include <muduo/base/mutex.h>
#include <muduo/net/callBacks.h>
#include <muduo/net/tcpConnection.h>
#include <boost/noncopyable.hpp>
#include <memory>
#include <atomic>

namespace muduo {
namespace net {

class connector;    // forward declaration
class event_loop;   // forward declaration

class tcp_client : boost::noncopyable {
public:
    tcp_client(event_loop* const loop, const inet_address& peer_addr, bool reconnect = false, string name = "");
    ~tcp_client();

    void start_connect();
    void stop_connect();
    void disconnect();
    const inet_address& get_remote_addr() const;

    tcp_connection::tcp_connection_ptr get_connection() const {
        mutexLock_guard locker(mutex_);
        return connHandle_;
    }

    void set_onMessage_callback(const onMsgCallback_t& cb) { onMsgCb_ = cb; }
    void set_onConnection_callback(const onConnectionCallback_t& cb) { connectionCb_ = cb; }
    void set_onWriteComplete_callback(const onWriteCompleteCallback_t& cb) { writeCompleteCb_ = cb; }

private:
    void initialize_new_connection(const int sockfd);
    void remove_connection(const tcp_connection::tcp_connection_ptr& conn);
    
private:
    event_loop* const loop_;
    std::unique_ptr<connector> connector_;
    string serverName_;
    std::atomic<bool> isConnect_;
    std::atomic<bool> reconnect_;   // when socket is closed, whether reconnect
    onConnectionCallback_t connectionCb_;
    onWriteCompleteCallback_t writeCompleteCb_;  
    onMsgCallback_t onMsgCb_;
    uint32_t nextID_;
    tcp_connection::tcp_connection_ptr connHandle_; // store connection, inside store member 'socket' Responsible for close(3) fd
    mutable mutexLock mutex_;   // protect to write/access tcp_connection_ptr        
};

} // namespace net
} // namespace muduo
#endif // MUDUO_NET_TCPCLIENT_H
