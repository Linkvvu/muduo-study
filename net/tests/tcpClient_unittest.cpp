#include <muduo/net/eventLoop.h>
#include <muduo/net/tcpClient.h>
#include <muduo/base/logging.h>
#include <muduo/net/timerId.h>
using namespace muduo;
using namespace muduo::net;

tcp_client* g_client;

void timeout() {
	LOG_INFO << "timeout";
	g_client->stop_connect();
}

int main(int argc, char* argv[]) {
	event_loop loop;
	inet_address serverAddr("127.0.0.1", 2); // no such server
	tcp_client client(&loop, serverAddr, "TcpClient");
	g_client = &client;
	loop.run_after(0.0, timeout);
	loop.run_after(10.0, std::bind(&event_loop::quit, &loop));
	client.start_connect();
	sleep(5);
	loop.loop();
}