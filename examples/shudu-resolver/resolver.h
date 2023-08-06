#if !defined(MUDUO_EXAMPLES_SHUDU_RESOLVER_H)
#define MUDUO_EXAMPLES_SHUDU_RESOLVER_H

#include <muduo/net/eventLoop.h>
#include <muduo/net/tcpServer.h>
#include <muduo/net/tcpClient.h>
#include <muduo/net/inetAddress.h>
#include <muduo/base/threadpool.h>

using namespace muduo;
using namespace muduo::net;

class shudu_resolver {
public:
    using tcp_connection_ptr = tcp_connection::tcp_connection_ptr;

    shudu_resolver(event_loop* const loop, const inet_address& serAddr, const int work_thread_num);

    void start() {
        server_.start();
        threadPool_.start(work_thread_num_);
    }

private:
    const int kCells = 81;
    void onMessage(const tcp_connection_ptr& conn, buffer* msg, TimeStamp recv_time);
    bool process_request(const tcp_connection_ptr& conn, const string& payload);
    void onNewConnection(const tcp_connection_ptr& conn);
    
private:
    event_loop* const loop_;
    const int work_thread_num_;
    tcp_server server_;
    threadpool threadPool_;
};

#endif // MUDUO_EXAMPLES_SHUDU_RESOLVER_H
