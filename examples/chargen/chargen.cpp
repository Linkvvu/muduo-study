#include <muduo/examples/chargen/chargen.h>
#include <muduo/net/eventLoop.h>
#include <muduo/base/logging.h>
#include <muduo/net/timerId.h>
using namespace muduo;
using namespace muduo::net;

chargen_server::chargen_server(event_loop* const loop, const inet_address& serAddr, bool print)
    : loop_(loop)
    , message_()
    , transferred_(0)
    , server_(loop, serAddr, "charGen server")
    , startTime_(std::chrono::steady_clock::now()) {
    
    server_.set_onConnection_callback([this](const tcp_connection::tcp_connection_ptr& conn) { this->onConnection(conn); });
    // server_.set_onMessage_callback([this](const tcp_connection::tcp_connection_ptr& conn, buffer* msg, TimeStamp recv_time) { this->onMessage(conn, msg, recv_time); });
    server_.set_onWriteComplete_callback([this](const tcp_connection::tcp_connection_ptr& conn) { this->onWriteComplete(conn); });

    if (print) {
        loop_->run_every(3.0, [this]() { this->printThroughput(); });
    }

    /// message_：
    /// 每行72个character
    /// 按顺序轮换、共94行
    string line;
    for (int i = 33; i < 127; i++) {
        line.push_back(static_cast<char>(i));
    }

    line += line;

    for (std::size_t i = 0; i < 127 - 33; ++i) {
        message_ += line.substr(i, 72) + '\n';
    }
}

void chargen_server::onConnection(const tcp_connection::tcp_connection_ptr& conn) {
    LOG_INFO << "ChargenServer - " << conn->get_peer_addr().ipAndPort() << " -> "
        << conn->get_local_addr().ipAndPort() << " is "
        << (conn->connected() ? "UP" : "DOWN");
    if (conn->connected()) {
        conn->set_tcp_noDelay(true);
        conn->send(message_);
    }
}

void chargen_server::onWriteComplete(const tcp_connection::tcp_connection_ptr& conn) {
    transferred_ += message_.size();
    conn->send(message_);
}


void chargen_server::printThroughput() {
    auto now = std::chrono::steady_clock::now();
    auto deff_time = now - startTime_;
    auto diff_seconds = std::chrono::duration_cast<std::chrono::seconds>(deff_time);
    auto throughput = transferred_/(1024ul*1024ul) / static_cast<uint64_t>(diff_seconds.count()); 
    startTime_ = now;
    transferred_ = 0;
    LOG_INFO << "Throughput: " << throughput << "Mbp/s";
}