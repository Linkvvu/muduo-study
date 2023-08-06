#if !defined(MUDUO_EXAMPLES_CHARGEN_CHARGEN_H)
#define MUDUO_EXAMPLES_CHARGEN_CHARGEN_H

#include <muduo/net/tcpServer.h>
#include <muduo/net/inetAddress.h>
#include <muduo/net/tcpConnection.h>
#include <string>
#include <chrono>

// RFC 864  char-set generator

using namespace muduo;
using namespace muduo::net;

class chargen_server {
public:
    chargen_server(event_loop* const loop, const inet_address& serAddr, bool print = false);
    void start() { server_.start(); }

private:
    void onConnection(const tcp_connection::tcp_connection_ptr& conn);

    void onMessage(const tcp_connection::tcp_connection_ptr& conn,
        buffer* buf,
        TimeStamp time);

    void onWriteComplete(const tcp_connection::tcp_connection_ptr& conn);

    void printThroughput();

private:
    event_loop* loop_;
    std::string message_;
    uint64_t transferred_;
    tcp_server server_;
    std::chrono::time_point<std::chrono::steady_clock> startTime_;
};

#endif // MUDUO_EXAMPLES_CHARGEN_CHARGEN_H
