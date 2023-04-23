// test: 两个线程分别创建各自的event-loop对象并开时loop

#include <muduo/base/Thread.h>
#include <muduo/net/eventLoop.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void threadFunc()
{
	printf("threadFunc(): pid = %d, tid = %d\n",
		getpid(), currentThread::tid());

	event_loop loop;
	loop.loop();
}

int main(void)
{
	printf("main(): pid = %d, tid = %d\n",
		getpid(), currentThread::tid());

	event_loop loop;

	Thread t(threadFunc);
	t.start();

	loop.loop();
	t.join();
	return 0;
}
