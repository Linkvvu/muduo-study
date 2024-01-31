/// Test single-thread environment
#include <muduo/TcpClient.h>

using namespace muduo;
using namespace std::chrono;

void timeout(const TcpClientPtr& client) {
    LOG_INFO << "Timeout, stop to connect";
    client->Stop();
}

int main() {
    EventLoop loop;
    InetAddr server_addr("127.0.0.1", 1);   // no such server
    TcpClientPtr client = CreateTcpClient(&loop, server_addr, "test-TcpClient");
    loop.RunAfter(milliseconds(0), [&client]() {
        timeout(client);
    });
    loop.RunAfter(duration_cast<milliseconds>(seconds(1)), [&client]() {
        client->GetEventLoop()->Quit();
    });
    client->Connect();  // starts to connect now
    loop.Loop();
}