#include <muduo/base/Thread.h>
#include <muduo/base/logging.h>
#include <muduo/net/eventLoop.h>

using namespace muduo;
using muduo::net::event_loop;
using muduo::Thread;

int main() {
	printf("threadFunc(): pid = %d, tid = %d\n",
		getpid(), currentThread::tid());
    event_loop loop;
    Thread t([&loop]() { loop.loop(); });
    t.start();
    t.join();
    return 0;
}