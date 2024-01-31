/// TcpClient destructs when TcpConnection is connected

#include <muduo/TcpClient.h>

using namespace muduo;
using namespace std::chrono;

void ThreadFunc(EventLoop* loop) {
    InetAddr server_addr("127.0.0.1", 8888);    // should succeed
    TcpClientPtr client = CreateTcpClient(loop, server_addr, "test-TcpClient");
    client->Connect();  // starts to connect now

    std::this_thread::sleep_for(seconds(1));
}

int main() {
    EventLoop loop;

    loop.RunAfter(seconds(3), [&loop]() {
        (&loop)->Quit();
    });

    std::thread trd([&loop]() {
        ThreadFunc(&loop);
    });

    loop.Loop();
    trd.join();
}