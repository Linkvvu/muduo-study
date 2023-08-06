#if !defined(MUDUO_EXAMPLES_TRANSMITTER_H)
#define MUDUO_EXAMPLES_TRANSMITTER_H

#include <muduo/net/tcpServer.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/tcpConnection.h>

using namespace muduo;
using namespace muduo::net;

class transmit_server {
public:
    static const size_t kDefault_highWark_makr = 1024*1024;    // 1M
    using tcp_connection_ptr = tcp_connection::tcp_connection_ptr;

    transmit_server(event_loop* const loop, const inet_address& serAddr, const string& target);
    void start() { server_.start(); }
private:
    void onNewConnection(const tcp_connection_ptr& conn);
    void onHighWaterMark(const tcp_connection_ptr& conn, size_t len);
    string read_content() const;
private:
    event_loop* const loop_;
    string filePath_;
    tcp_server server_;
};

#endif // MUDUO_EXAMPLES_TRANSMITTER_H
