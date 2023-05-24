#include <muduo/net/acceptor.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/tcpServer.h>
#include <muduo/net/tcpServer.h>
#include <muduo/net/socketsOPs.h>
#include <muduo/net/inetAddress.h>
#include <muduo/net/tcpConnection.h>
#include <muduo/base/logging.h>

using namespace muduo;
using namespace muduo::net;

tcp_server::tcp_server(event_loop* const loop, const inet_address& listener_addr, string name)
    : loop_(loop)
    , serverName_(name)
    , ipAndPort_(listener_addr.ipAndPort())
    , acceptor_(std::make_unique<acceptor>(loop, listener_addr))
    , started_(false)
    , onMsgCb_(nullptr)
    , onConnCb_(nullptr)
    , nextConnId_(1)
    , conns_() {
        acceptor_->set_newConn_callback(std::bind(&tcp_server::handle_newConnection, this, std::placeholders::_1, std::placeholders::_2));
    }

tcp_server::~tcp_server() {
    loop_->assert_loop_in_hold_thread();
    LOG_DEBUG << "tcp-serve [" << this << "] start exit service now"; 
    for (auto& item : conns_) {
        tcp_connection_ptr conn = item.second;  // shard_ptr count +1(total 2)
        item.second.reset();                    // shard_ptr count -1(total 1)

        conn->get_event_loop()->run_in_eventLoop_thread([conn]() {  // shard_ptr count +1(total 2)
            conn->connection_destroy();
        });
    }
}


void tcp_server::handle_newConnection(int sockfd, const inet_address& peer_addr) {
    loop_->assert_loop_in_hold_thread();
    char tmp_buf[32] = {0};
    ::snprintf(tmp_buf, sizeof tmp_buf, ":%s#%d", ipAndPort().c_str(), nextConnId_++);
    string conn_name = serverName_ + tmp_buf;
    LOG_INFO << "Server[" << serverName_ << "] - New connection: [" << conn_name << "] from " << peer_addr.ipAndPort();
    inet_address local_addr(sockets::get_local_addr(sockfd));
    tcp_connection_ptr new_conn = std::make_shared<tcp_connection>(loop_, conn_name, sockfd, local_addr, peer_addr);
    conns_[conn_name] = new_conn;   // insert into connection list
    new_conn->set_onConnection_callback(onConnCb_);
    new_conn->set_onMessage_callback(onMsgCb_);
    new_conn->set_onClose_callback(std::bind(&tcp_server::remove_connection, this, std::placeholders::_1));

    new_conn->step_into_established();
}

void tcp_server::remove_connection(const std::shared_ptr<tcp_connection>& conn) {
    loop_->assert_loop_in_hold_thread();
    auto ret = conns_.erase(conn->name());
    assert(ret == 1);
    conn->connection_destroy();
}

void tcp_server::start() {
    if (!started_)
        started_ = true;
    if (!acceptor_->listening()) {
        loop_->run_in_eventLoop_thread([this]() { this->acceptor_->listen(); });
    }
}
