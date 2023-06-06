#if !defined(MUDUO_NET_CONNECTOR_H)
#define MUDUO_NET_CONNECTOR_H

#include <muduo/net/callBacks.h>
#include <muduo/net/inetAddress.h>
#include <boost/noncopyable.hpp>
#include <cstdlib>
#include <atomic>
#include <memory>
#include <functional>

namespace muduo {
namespace net {

class event_loop;   // forward declaration
class channel;   // forward declaration

class connector : boost::noncopyable {
public:
    using newConnCallback_t = std::function<void(int sockfd)>;
    const uint64_t KMaxDelayTime_ms = 30000;   // 3s

    connector(event_loop* const loop, const inet_address& raddr, const newConnCallback_t& func);
    ~connector();

    /// @brief start connect to peer end
    void start_connect();
    /// @brief if currently is connecting, stop it
    void stop_connect();
    /// @note Cannot be invoked across threads
    void restart_connect();

    const inet_address& get_remote_addr() const
    { return peerAddr_; }
    
private:
    enum class stage { connecting, connected, disconnected };
    static const uint64_t KInitRetryDelayMs = 500;

    void start_connect_in_loop();
    void connect();                 // invoke connect(3)
    void connecting(int sockfd);    // set "write event" in the channel 
    void handle_write();            // Check whether connect(3) succeeded
    void handle_error();            // deal 'POLLERR' event on the related fd
    void retry(int sockfd);
    int remove_and_resetChannel();  // remove "write event" and set channel to null
    void stop_connect_in_loop();    

private:
    event_loop* const loop_;
    inet_address peerAddr_;
    std::unique_ptr<channel> helperChannel_;    // help connector attention to connect(2) result
    newConnCallback_t newConnCb_;               // callback for after success connect 
    uint64_t delayTime_;                        // reconnect delay time, units: ms
    std::atomic<bool> isConnect_;               // whether start connect
    std::atomic<stage> stage_;                  // connection`s current stage
};

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_CONNECTOR_H
