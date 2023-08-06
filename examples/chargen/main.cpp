#include <muduo/examples/chargen/chargen.h>
#include <muduo/base/logging.h>
#include <muduo/net/eventLoop.h>
using namespace muduo;
using namespace muduo::net;

int main() {
	set_log_level(logger::DEBUG);

	event_loop loop;
	inet_address listenAddr(8888);
	chargen_server server(&loop, listenAddr, true);
	server.start();

	loop.loop();
}

