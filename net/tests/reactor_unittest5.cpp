#include <muduo/net/eventLoop.h>
#include <muduo/base/currentThread.h>
#include <muduo/net/timerId.h>
#include <thread>
using namespace muduo;
using namespace muduo::net;
using namespace muduo::currentThread;

event_loop* g_loop;
int g_flag = 0;

void foo1();
void foo2();
void foo3();
void foo4();

void foo1() {
    g_flag = 1;
    printf("foo1(): tid = %d, Flag = %d\n", tid(), g_flag);
    g_loop->run_in_eventLoop(foo2);
    g_flag = 2;
}

void foo2() {
    printf("foo2(): tid = %d, Flag = %d\n", tid(), g_flag);
    g_loop->enqueue_eventLoop(foo3);
}

void foo3() {
    printf("foo3(): tid = %d, Flag = %d\n", tid(), g_flag);
    g_loop->run_after(3, foo4);
    g_flag = 3;
}

void foo4() {
    printf("foo4(): tid = %d, Flag = %d\n", tid(), g_flag);
    g_loop->quit();
}

int main() {
    printf("main(): tid = %d, Flag = %d\n", tid(), g_flag); 
    event_loop loop;
    g_loop = &loop;

    std::thread t(
        [&loop]() {
            printf("another thread: tid = %d, Flag = %d\n", tid(), g_flag);
            loop.run_after(1, foo1);
            printf("another thread: tid = %d, Flag = %d success to add task\n", tid(), g_flag);
        }
    );
    t.join();
    // loop.run_after(1, foo1);
    loop.loop();
    printf("main(): tid = %d, Flag = %d\n", tid(), g_flag); 
}