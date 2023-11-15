/// single-Reactor mode, Acceptor and IO-handler run in same thread
#include <muduo/InetAddr.h>
#include <muduo/TcpServer.h>
#include <muduo/EventLoop.h>
#include <muduo/TcpConnection.h>
#include <muduo/Callbacks.h>
#include <memory>
#include <iostream>

using namespace muduo;

EventLoop* g_loop;

class Test {
public:
    Test(EventLoop* loop, const InetAddr& listenerAddr) : s_(loop, listenerAddr, "Test")
    {
        s_.SetConnectionCallback(std::bind(&Test::onConnection, this, std::placeholders::_1));
        s_.SetOnMessageCallback([](const TcpConnectionPtr& conn, Buffer* buf, ReceiveTimePoint_t recv_timepoint) {
            auto response = buf->RetrieveAllAsString();
            std::cout << "recv from [" << conn->GetName() << "]: " << response;
        });
    }

    void start() { s_.ListenAndServe(); }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->IsConnected()) {
            printf("onConnection(): new connection [%s] from %s, local address: %s\n",
                conn->GetName().c_str(),
                conn->GetRemoteAddr().GetIpPort().c_str(),
                conn->GetLocalAddr().GetIpPort().c_str());
        } else {
            printf("onConnection(): connection [%s] is down\n",
                conn->GetName().c_str());
            g_loop->Quit();
        }
    }


private:
    TcpServer s_;
};


int main() {
    EventLoop loop;
    g_loop = &loop;
    
    InetAddr listener_addr(8888);
    Test t(g_loop, listener_addr);
    t.start();
    
    loop.Loop();
}