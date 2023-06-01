/// echo server
#include <muduo/net/tcpServer.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/tcpConnection.h>
#include <memory>
#include <thread>

using namespace muduo::net;

event_loop* g_loop;

class Test {
public:
    Test(event_loop* loop, const inet_address& listener_addr) : s_(loop, listener_addr, "Test-Server")
    {
        s_.set_IO_threadInit_callback([this](event_loop* loop) { this->IO_threadInit_callback(loop); });
        s_.set_onConnection_callback(std::bind(&Test::onConnection, this, std::placeholders::_1));
        s_.set_onMessage_callback(std::bind(&Test::onMessages, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }
    void start() { s_.start(); }
    void set_IO_threadNum(const int n) { s_.set_IO_threadNum(n); }

private:
    void IO_threadInit_callback(event_loop* loop) {
        static int count = 0;
        std::printf("--- The Initialize-callback is called In loop %p <count: %d>\n", loop, ++count);
    }

    void onConnection(const std::shared_ptr<tcp_connection>& conn) {
        if (conn->connected()) {
            printf("--- onConnection(): new connection [%s] from %s, local address: %s\n",
                conn->name().c_str(),
                conn->get_peer_addr().ipAndPort().c_str(),
                conn->get_local_addr().ipAndPort().c_str());
        } else {
            printf("--- onConnection(): connection [%s] is down\n",
                conn->name().c_str());
        }
    }
    
    void onMessages(const std::shared_ptr<tcp_connection>& conn, buffer* msg, muduo::TimeStamp receiveTime) {
        std::string echo_data = msg->retrieve_all_as_string();
        printf("--- onMessage(): received %zd bytes from connection [%s] at %s\n",
            echo_data.size(),
            conn->name().c_str(),
            receiveTime.toFormattedString().c_str());
        std::string data = "[server recv msg at: " + receiveTime.toFormattedString(false) + "] " + echo_data; 
        std::thread t([conn, data]() {
            conn->send(data);
            std::printf("--- send be invoke\n");
            conn->shutdown();
        });
        t.detach();
    }

    void onWriteComplete(const std::shared_ptr<tcp_connection>& conn) {

    }

private:
    tcp_server s_;
};

int main() {
    event_loop main_loop;
    g_loop = &main_loop;

    inet_address listenerAddr(8888);
    Test t(g_loop, listenerAddr);
    t.set_IO_threadNum(2);
    t.start();

    main_loop.loop();
    std::printf("main function exit\n");
}