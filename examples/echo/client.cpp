#include <muduo/net/channel.h>
#include <muduo/net/tcpClient.h>
#include <muduo/base/logging.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/inetAddress.h>
// #include <muduo/net/tcpConnection.h>
#include <functional>
#include <iostream>

using namespace muduo;
using namespace muduo::net;


class TestClient {
public:
    TestClient(event_loop* loop, const inet_address& listenAddr)
        : loop_(loop),
        client_(loop, listenAddr, "TestClient"),
        stdinChannel_(loop, 0) {
        client_.set_onConnection_callback(
            std::bind(&TestClient::onConnection, this, std::placeholders::_1));
        // client_.set_onMessage_callback(
        //     std::bind(&TestClient::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        client_.set_onMessage_callback([this](const tcp_connection::tcp_connection_ptr& conn, buffer* buf, TimeStamp time) {
            this->onMessage(std::move(conn), buf, time);
        });
        //client_.enableRetry();
        // 标准输入缓冲区中有数据的时候，回调TestClient::handleRead
        stdinChannel_.setReadCallback(std::bind(&TestClient::handleRead, this));
        stdinChannel_.enableReading();		// 关注stdin可读事件
    }

    ~TestClient() {
        LOG_INFO << "~TsetClient is destructed!";
    }

    void connect() {
        client_.start_connect();
    }

private:
    void onConnection(const tcp_connection::tcp_connection_ptr& conn) {
        if (conn->connected())
            std::cout << " -- new connection[" << conn->name() << "] is coming, from " << conn->get_peer_addr().ipAndPort() << std::endl;     
        else
            std::cout << " -- connection[" << conn->name() << "] is down" << std::endl;
    }

    void onMessage(const tcp_connection::tcp_connection_ptr& conn, buffer* buf, TimeStamp time) {
        string msg(buf->retrieve_all_as_string());
        std::cout << conn.use_count() << std::endl;
        LOG_INFO << conn->name() << " recv " << msg.size() << " bytes at " << time.toFormattedString();
        printf("%s\n", msg.c_str());
    }

    // 标准输入缓冲区中有数据的时候，回调该函数
    void handleRead() {
        char buf[1024] = { 0 };
        std::cin.get(buf, sizeof buf);
        std::cin.get();
        // buf[std::strlen(buf) - 1] = '\0';		// 去除\n
        client_.get_connection()->send(buf);
    }

    event_loop* loop_;
    tcp_client client_;
    channel stdinChannel_;		// 标准输入Channel
};

int main(int argc, char* argv[]) {
    event_loop loop;

    inet_address serverAddr("127.0.0.1", 8888);
    TestClient client(&loop, serverAddr);
    client.connect();

    loop.loop();
}