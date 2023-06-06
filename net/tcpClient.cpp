#include <muduo/net/tcpClient.h>
#include <muduo/net/connector.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/socketsOPs.h>
#include <muduo/base/logging.h>

using namespace muduo;
using namespace muduo::net;

namespace muduo {
namespace net {
namespace detail {

void remove_connection(const tcp_connection::tcp_connection_ptr& conn) {
    conn->get_event_loop()->run_in_eventLoop_thread([conn]() { conn->connection_destroy(); });
}

} // namespace detail 
} // namespace net
} // namespace mudu 


const inet_address& tcp_client::get_remote_addr() const {
    return connector_->get_remote_addr();
}

tcp_client::tcp_client(event_loop* const loop, const inet_address& peer_addr, bool reconnect, string name)
    : loop_(loop)
    , connector_(std::make_unique<connector>(loop, peer_addr, [this](const int sockfd) { this->initialize_new_connection(sockfd); }))
    , serverName_(name)
    , isConnect_(false)
    , reconnect_(reconnect)
    /* , every callback(nullptr) */
    , nextID_(1)
    , connHandle_(nullptr) {}

tcp_client::~tcp_client() { 
    tcp_connection::tcp_connection_ptr tie;

    {   // It is possible that the tcp_client object is not in the same thread as the loop object, so use mutex
        mutexLock_guard locker(mutex_);
        tie = connHandle_;  // shard_ptr count+1
    }
    
    if (tie) {
        loop_->run_in_eventLoop_thread([tie]() {    // shard_ptr count+1
            tie->set_onClose_callback(
                [](const tcp_connection::tcp_connection_ptr& conn) { detail::remove_connection(conn); }
            );
        });
        // tcp_client::~tcp_client调用完毕后，会析构成员connHandle_
        // 若该tcp_client对象与loop不是同一线程下的，那么45行会使得shard_ptr count+1
        // 此时析构成员connHandle_只是将连接的count-1，并不会调用其所管理对象的析构
        // 当连接的count为0时，会被析构
        // 又由于其内部存在'socket'成员，它管理着fd的生命周期，当'socket'成员被析构时，fd会被close(3)
    } else {
        if (isConnect_) {   // 如果正在连接，则停止连接
            connector_->stop_connect();
        }
    }
}

void tcp_client::start_connect() {
    isConnect_ = true;
    connector_->start_connect();
}

void tcp_client::stop_connect() {
    isConnect_ = false;
    connector_->stop_connect();
}

void tcp_client::disconnect() {
    isConnect_ = false;
    assert(connHandle_.operator bool());    // assert shard_ptr stored a connection now
 
    {
        mutexLock_guard locker(mutex_);
        if (connHandle_) {
            connHandle_->shutdown();
        }
    }
    
}

void tcp_client::initialize_new_connection(const int sockfd) {
    loop_->assert_loop_in_hold_thread();
    inet_address local_addr = sockets::get_local_addr(sockfd);
    inet_address peer_addr =  sockets::get_peer_addr(sockfd);
    char tmp_buf[50] = {0};
    std::snprintf(tmp_buf, sizeof tmp_buf, ":%s#%u", peer_addr.ipAndPort().c_str(), nextID_++);
    string conn_name = serverName_ + tmp_buf;

    tcp_connection::tcp_connection_ptr new_conn(std::make_shared<tcp_connection>(loop_, conn_name, sockfd, local_addr, peer_addr));
    new_conn->set_onConnection_callback(connectionCb_);
    new_conn->set_onMessage_callback(onMsgCb_);
    new_conn->set_onWriteComplete_callback(writeCompleteCb_);
    new_conn->set_onClose_callback(
        [this](const tcp_connection::tcp_connection_ptr& conn) { this->remove_connection(conn); });

    {
        mutexLock_guard locker(mutex_);
        connHandle_ = new_conn;
    }
    new_conn->step_into_established();
}

void tcp_client::remove_connection(const tcp_connection::tcp_connection_ptr& conn) {
    loop_->assert_loop_in_hold_thread();
    assert(loop_ == conn->get_event_loop());
    loop_->run_in_eventLoop_thread([conn]() { conn->connection_destroy(); });   // must be invoked in here

    {
        mutexLock_guard locker(mutex_);
        connHandle_.reset();
    }
    
    if (reconnect_ && isConnect_) {
        LOG_INFO << "tcp_client connection[" << conn->name() << "] reconnecting to " << conn->get_peer_addr().ipAndPort();       
        connector_->restart_connect();
    }
}