#if !defined(MUDUO_NET_TCPCONNECTION_H)
#define MUDUO_NET_TCPCONNECTION_H

#include <boost/noncopyable.hpp>
#include <muduo/net/callBacks.h>
#include <muduo/net/inetAddress.h>
#include <memory>

namespace muduo {

class TimeStamp;    // forward declaration

namespace net{

class event_loop;   // forward declaration
class socket;       // forward declaration
class channel;      // forward declaration

class tcp_connection : private boost::noncopyable
                    , public std::enable_shared_from_this<tcp_connection> {
public:
    enum class stage { connecting, connected, disconnected };

    explicit tcp_connection(event_loop* const loop, const string& name, const int sockfd, const inet_address& localAddr, const inet_address& peerAddr);
    ~tcp_connection();
    void set_onMessage_callback(const onMsgCallback_t& cb) { onMsgCb_ = cb; }
    void set_onConnection_callback(const onConnectionCallback_t& cb) { onConnCb_ = cb; }
    void set_onClose_callback(const onCloseCallback_t& cb) { onCloseCb_ = cb; }
    void step_into_established();   // should be invoke once
    void connection_destroy();      // should be invoke once

    void set_stage(const stage s) { stage_ = s; } 

    event_loop* get_event_loop() { return loop_; }
    string name() const { return name_; }
    bool connected() const { return stage_ == stage::connected; }
    const inet_address& get_local_addr() const { return localAddr_; }
    const inet_address& get_peer_addr() const { return peerAddr_; }

private:
    using tcp_connection_ptr = std::shared_ptr<tcp_connection>;
    void handle_read(TimeStamp recv_time);
    void handle_close();
    void handle_error();

private:
    event_loop* const loop_;                // belong to which event-loop
    string name_;                           // ID for tcp-server connections list
    std::unique_ptr<socket> connSocket_;    // owns current connection file descriptor
    std::unique_ptr<channel> connChannel_;  // Focus on the status and respond event of current connection 
    tcp_connection::stage stage_;           // the connection`s stage
    inet_address localAddr_;
    inet_address peerAddr_;
    onMsgCallback_t onMsgCb_;
    onConnectionCallback_t onConnCb_;       // connect or close connection callback
    onCloseCallback_t onCloseCb_;
};

} // namespace net
} // namespace muduo 

#endif // MUDUO_NET_TCPCONNECTION_H
