#include <muduo/net/eventLoopThread.h>
#include <muduo/net/eventLoop.h>
#include <muduo/base/currentThread.h>
#include <thread>
using namespace muduo;
using namespace muduo::net;
using namespace muduo::currentThread;

event_loop* g_loop;

int main() {
    printf("print(): pid = %d, tid = %d\n", ::getpid(), muduo::currentThread::tid());

    // 由新的线程调用start_loop，且eventLoop_thread对象立刻被析构，测试eventLoop_thread::destructor与该对象新创建的IO线程间的"尽态条件"
    {
        eventLoop_thread t(nullptr, "event-loop thread");
        Thread tmp([&t]() { g_loop = t.start_loop(); });
        tmp.start();
    }
    sleep(5);
}