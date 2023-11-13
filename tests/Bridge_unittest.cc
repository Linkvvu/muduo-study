#include <EventLoop.h>
#include <thread>
#include <unistd.h>

using namespace muduo;

EventLoop* g_loop = nullptr;
int g_flag = 0;

extern void foo1();
extern void foo2();
extern void foo3();
extern void foo4();

void foo1() {
    g_flag = 1;
    LOG_INFO << "foo1(): tid=" << ::pthread_self() << ", Flag=" << g_flag;
    g_loop->RunInEventLoop(foo2);
    g_flag = 2;
}

void foo2() {
    LOG_INFO << "foo2(): tid=" << ::pthread_self() << ", Flag=" << g_flag;
    g_loop->EnqueueEventLoop(foo3);
}

void foo3() {
    LOG_INFO << "foo3(): tid=" << ::pthread_self() << ", Flag=" << g_flag;
    g_loop->RunAfter(std::chrono::seconds(3), foo4);
    g_flag = 3;
}

void foo4() {
    LOG_INFO << "foo4(): tid=" << ::pthread_self() << ", Flag=" << g_flag;
    g_loop->Quit();
}

int main() {
    LOG_INFO << "main(): tid=" << ::pthread_self() << ", Flag=" << g_flag;
    EventLoop loop;
    g_loop = EventLoop::GetCurrentThreadLoop();

    std::thread t(
        [&loop]() {
            LOG_INFO << "another thread: tid=" << ::pthread_self() << ", Flag=" << g_flag;
            loop.RunAfter(std::chrono::seconds(1), foo1);
            LOG_INFO << "another thread: tid=" << ::pthread_self() << ", Flag=" << ", success to add task" << g_flag;
        }
    );
    t.join();
    loop.Loop();
    LOG_INFO << "main(): tid=" << ::pthread_self() << ", Flag=" << g_flag;
}