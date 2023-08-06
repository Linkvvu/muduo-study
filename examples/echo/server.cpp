#include <muduo/net/eventLoop.h>
#include <muduo/net/tcpServer.h>
#include <muduo/net/tcpConnection.h>
#include <muduo/base/logging.h>
#include <iostream>
using namespace muduo;
using namespace muduo::net;

event_loop* g_loop;

class echo_server {
public:
    echo_server(event_loop* loop, const inet_address& serAddr)
        : server_(loop, serAddr, "echo-server")
        , loop_(loop) {
            server_.set_onConnection_callback([this](const tcp_connection::tcp_connection_ptr& conn) { this->onNewConnection(conn); });
            server_.set_onMessage_callback([this](const tcp_connection::tcp_connection_ptr& conn, buffer* msg, muduo::TimeStamp receiveTime) {
                this->onMessage(conn, msg, receiveTime);
            });
        }

    void start() { server_.start(); }
    
private:
    void onNewConnection(const tcp_connection::tcp_connection_ptr& conn) {
        if (conn->connected())
            std::cout << " -- new connection[" << conn->name() << "] is coming, from " << conn->get_peer_addr().ipAndPort() << std::endl;     
        else
            std::cout << " -- connection[" << conn->name() << "] is down" << std::endl;
    }

    void onMessage(const tcp_connection::tcp_connection_ptr& conn, buffer* msg, muduo::TimeStamp receiveTime) {
        std::string echo_data = msg->retrieve_all_as_string();
        printf(" -- onMessage(): received %zd bytes from connection [%s] at %s\n",
            echo_data.size(),
            conn->name().c_str(),
            receiveTime.toFormattedString().c_str());
        std::string data = "[server recv msg at: " + receiveTime.toFormattedString(false) + "] " + echo_data; 
        conn->send(data);
    }

private:
    tcp_server server_;
    event_loop* const loop_;
};

int main() {
    event_loop loop;
    g_loop = &loop;

    inet_address server_address(8888);
    echo_server server(g_loop, server_address);
    server.start();
    loop.loop();

    return 0;
}