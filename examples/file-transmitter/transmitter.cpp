#include <muduo/examples/file-transmitter/transmitter.h>
#include <muduo/base/logging.h>
#include <fstream>

using namespace muduo;
using namespace muduo::net;

transmit_server::transmit_server(event_loop* const loop, const inet_address& serAddr, const string& target)
    : loop_(loop)
    , filePath_(target)
    , server_(loop, serAddr, "file-transmitter") {
        // std::ifstream()
        server_.set_onHighWaterMark_callback([this](const tcp_connection_ptr& conn, size_t len) {
            this->onHighWaterMark(conn, len);
        }, kDefault_highWark_makr);
        server_.set_onConnection_callback([this](const tcp_connection_ptr& conn) { this->onNewConnection(conn); });
    }

void transmit_server::onHighWaterMark(const tcp_connection_ptr& conn, size_t len) {
    LOG_INFO << "arrive HighWater-mark " << len;
}

void transmit_server::onNewConnection(const tcp_connection_ptr& conn) {
    LOG_INFO << "transmit_server - " << conn->get_peer_addr().ipAndPort() << " -> "
           << conn->get_local_addr().ipAndPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
    
    if (conn->connected()) {    // 如果是新的连接
        LOG_INFO << "transmit_server - sending file " << filePath_ << " to " << conn->get_peer_addr().ipAndPort();
        string file_content = read_content();
        conn->send(file_content);
        conn->shutdown();
        LOG_INFO << "transmit_server - done";
    }
}

// inefficient
string transmit_server::read_content() const {
    using std::ios_base;
    std::ifstream ifs(filePath_, ios_base::in|ios_base::binary);

    if (ifs.is_open()) {
        string content;
        const int kbufSize = 1024*1024; // buffer size: 1M
        char iobuf[kbufSize] = {0};
        ifs.rdbuf()->pubsetbuf(iobuf, kbufSize);   // set underlying buffer

        char tmp[1024*5] = {0};
        while (ifs.read(tmp, sizeof tmp)) {
            content.append(tmp, ifs.gcount());
        }
        content.append(tmp, ifs.gcount());
        ifs.close();
        return content;
    }

    return "";
}