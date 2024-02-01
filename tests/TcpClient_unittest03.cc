// TcpClient destructs in a different thread.
#include <muduo/TcpClient.h>
#include <muduo/EventLoopThread.h>

using namespace muduo;
using namespace std::chrono;

int main() {
    EventLoopThread loop_thread;
    {
        InetAddr server_addr("127.0.0.1", 8888);    // should succeed
        TcpClientPtr client = CreateTcpClient(loop_thread.Run(), server_addr, "test-TcpClient");
        client->Connect();
        std::this_thread::sleep_for(milliseconds(500));
        client->Shutdown();
    }
    std::this_thread::sleep_for(seconds(1));
}