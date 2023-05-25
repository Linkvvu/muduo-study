#include <muduo/net/tcpServer.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/inetAddress.h>
#include <muduo/net/tcpConnection.h>
#include <memory>

using namespace muduo;

net::event_loop* g_loop;

class Test {
public:
    Test(net::event_loop* loop, const net::inet_address& listenerAddr) : s_(loop, listenerAddr, "Test-Server")
    {
        s_.set_onConnection_callback(std::bind(&Test::onConnection, this, std::placeholders::_1));
    }

    void start() { s_.start(); }

private:
    void onConnection(const std::shared_ptr<net::tcp_connection>& conn) {
        if (conn->connected()) {
            printf("onConnection(): new connection [%s] from %s, local address: %s\n",
                conn->name().c_str(),
                conn->get_peer_addr().ipAndPort().c_str(),
                conn->get_local_addr().ipAndPort().c_str());
        } else {
            printf("onConnection(): connection [%s] is down\n",
                conn->name().c_str());
            g_loop->quit();
        }
    }


private:
    net::tcp_server s_;
};


int main() {
    using namespace net;
    event_loop loop;
    g_loop = &loop;

    inet_address listener_addr(8888);

    Test t(g_loop, listener_addr);
    t.start();
    
    loop.loop();
}