/**
 * eventLoop_threadPool class is added to tcp-server
*/

#include <muduo/net/tcpServer.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/inetAddress.h>
#include <muduo/net/tcpConnection.h>
#include <memory>

using namespace muduo::net;

event_loop* g_loop;

class Test {
public:
    Test(event_loop* loop, const inet_address& listener_addr) : s_(loop, listener_addr, "Test-Server")
    {
        s_.set_IO_threadInit_callback([this](event_loop* loop) { this->IO_threadInit_callback(loop); });
        s_.set_onConnection_callback(std::bind(&Test::onConnection, this, std::placeholders::_1));
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
            g_loop->quit();
        }
    }

private:
    tcp_server s_;
};



int main() {
    event_loop main_loop;
    g_loop = &main_loop;

    inet_address listenerAddr(8888);
    Test t(g_loop, listenerAddr);
    t.set_IO_threadNum(3);
    t.start();

    main_loop.loop();
    std::printf("progress exit");
}