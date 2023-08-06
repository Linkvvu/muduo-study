#include <muduo/base/logging.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/inetAddress.h>
#include <muduo/net/tcpClient.h>

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

class ChargenClient : boost::noncopyable {
public:
    ChargenClient(event_loop* loop, const inet_address& listenAddr)
        : loop_(loop),
        client_(loop, listenAddr, false, "ChargenClient") {
        client_.set_onConnection_callback(
            boost::bind(&ChargenClient::onConnection, this, _1));
        client_.set_onMessage_callback(
            boost::bind(&ChargenClient::onMessage, this, _1, _2, _3));
        // client_.enableRetry();
    }

    void connect() {
        client_.start_connect();
    }

private:
    void onConnection(const tcp_connection::tcp_connection_ptr& conn) {
        LOG_INFO << conn->get_local_addr().ipAndPort() << " -> "
            << conn->get_peer_addr().ipAndPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");

        if (!conn->connected())
            loop_->quit();
    }

    void onMessage(const tcp_connection::tcp_connection_ptr& conn, buffer* buf, TimeStamp receiveTime) {
        buf->retrieve_all();
    }

    event_loop* loop_;
    tcp_client client_;
};

int main(int argc, char* argv[]) {
    LOG_INFO << "pid = " << getpid();
    if (argc > 1) {
        event_loop loop;
        inet_address serverAddr(argv[1], 8888);

        ChargenClient timeClient(&loop, serverAddr);
        timeClient.connect();
        loop.loop();
    } else {
        printf("Usage: %s host_ip\n", argv[0]);
    }
}

