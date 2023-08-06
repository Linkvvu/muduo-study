#include <muduo/net/tcpServer.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/tcpConnection.h>
#include <muduo/base/logging.h>
#include <iostream>
#include <fstream>
#include <memory>

using namespace muduo;
using namespace muduo::net;
using std::ios_base;

class transmit_server {
public:
    static const size_t kDefault_highWark_makr = 1024*1024;    // 1M
    static const size_t kBufSize = 1024;
    using tcp_connection_ptr = tcp_connection::tcp_connection_ptr;

    transmit_server(event_loop* const loop, const inet_address& serAddr, const string& target)
        : loop_(loop)
        , filePath_(target)
        , server_(loop, serAddr, "file-transmitter")
        , ifs_(std::make_unique<std::ifstream>(target, ios_base::in|ios_base::binary)) {
            // std::ifstream()
            if (!ifs_->is_open()) {
                exit(EXIT_FAILURE);
            }

            server_.set_onHighWaterMark_callback([this](const tcp_connection_ptr& conn, size_t len) {
                this->onHighWaterMark(conn, len);
            }, kDefault_highWark_makr);
            server_.set_onConnection_callback([this](const tcp_connection_ptr& conn) { this->onNewConnection(conn); });
            server_.set_onWriteComplete_callback([this](const tcp_connection_ptr& conn) { this->onWriteComplete(conn); });
    }

    void start() { server_.start(); }

private:
    void onNewConnection(const tcp_connection_ptr& conn) {
        LOG_INFO << "transmit_server - " << conn->get_peer_addr().ipAndPort() << " -> "
           << conn->get_local_addr().ipAndPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
    
        if (conn->connected()) {    // 如果是新的连接
            LOG_INFO << "transmit_server - sending file " << filePath_ << " to " << conn->get_peer_addr().ipAndPort();
            char buf[kBufSize] = {0};
            ifs_->read(buf, kBufSize);
            conn->send(buf, ifs_->gcount());
            LOG_INFO << "transmit_server - done";
        }
    }

    void onHighWaterMark(const tcp_connection_ptr& conn, size_t len)  {
        LOG_INFO << "arrive HighWater-mark " << len;
    }


    void onWriteComplete(const tcp_connection_ptr& conn) {
        char buf[kBufSize] = {0};
        ifs_->read(buf, kBufSize);
        if (ifs_->gcount() > 0) {
            conn->send(buf, ifs_->gcount());
        } else {
            if (ifs_->eof()) {
                LOG_INFO << "transmit_server - done";
            } else {
                LOG_ERROR << "ifstream occur a err";
            }
            conn->shutdown();
        }
    }

private:
    event_loop* const loop_;
    string filePath_;
    tcp_server server_;
    std::unique_ptr<std::ifstream> ifs_;
};

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cerr << "Usage: " << argv[0] << " <file_for_downloading>" << std::endl;
        return -1;
    }

    event_loop loop;
    inet_address ser_addr(8888);
    transmit_server transmitter(&loop, ser_addr, argv[1]);

    transmitter.start();
    loop.loop();    

    return 0;    
}