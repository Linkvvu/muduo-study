#include <muduo/net/eventLoopThread.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/timerId.h>

muduo::net::event_loop* g_loop;

void print() {
    printf("print(): pid = %d, tid = %d\n", ::getpid(), muduo::currentThread::tid());
}

int main() {
    using muduo::net::eventLoop_thread;
    printf("print(): pid = %d, tid = %d\n", ::getpid(), muduo::currentThread::tid());

    eventLoop_thread t(nullptr, "event-loop thread");
    g_loop = t.start_loop();
    // 异步调用print(), 即不用等待print()操作完成，仅将print()添加到loop对象所在IO线程，让该IO线程执行
    g_loop->run_in_eventLoop_thread(&print);
    sleep(1);
    // run_after内部也调用了run_in_eventLoop_thread()，故也是异步调用
    g_loop->run_after(2, &print);
    sleep(3);
    g_loop->quit();
    printf("exit main().\n");
}