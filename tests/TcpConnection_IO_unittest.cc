#include <TcpConnection.h>
#include <EventLoop.h>
#include <TcpServer.h>
#include <functional>
#include <iostream>
#include <string>

using namespace muduo;

EventLoop* g_loop;

class EchoServer {
public:
    EchoServer(EventLoop* loop, const InetAddr& listenerAddr)
        : s_(loop, listenerAddr, "Echo")
    {
        s_.SetConnectionCallback(std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        s_.SetOnMessageCallback(std::bind(&EchoServer::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        s_.SetOnWriteCompleteCallback(std::bind(&EchoServer::OnWriteComplete, this, std::placeholders::_1));
    }

    void Start()
    { s_.ListenAndServe(); }
    
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
        }
    }

    void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, ReceiveTimePoint_t recv_timepoint) {
        auto response = buf->RetrieveAllAsString();
        std::cout << "recv from [" << conn->GetName() << "]: " << response;
        conn->Send(response);
    }

    void OnWriteComplete(const TcpConnectionPtr& conn) {
        std::cout << "connection [" << conn->GetName() << "] has written a response to peer." << std::endl;
    }

private:
    TcpServer s_;
};

int main() {
    EventLoop loop;
    g_loop = &loop;
    
    InetAddr listener_addr(8888);
    EchoServer t(g_loop, listener_addr);
    t.Start();
    
    loop.Loop();
}