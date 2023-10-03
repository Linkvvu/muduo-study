#include <EventLoop.h>
#include <fmt/core.h>
#include <fmt/ostream.h>
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
    fmt::println("foo1(): tid={0}, Flag={1}", fmt::streamed(std::this_thread::get_id()), g_flag);
    g_loop->RunInEventLoop(foo2);
    g_flag = 2;
}

void foo2() {
    fmt::println("foo2(): tid={0}, Flag={1}", fmt::streamed(std::this_thread::get_id()), g_flag);
    g_loop->EnqueueEventLoop(foo3);
}

void foo3() {
    fmt::println("foo3(): tid={0}, Flag={1}", fmt::streamed(std::this_thread::get_id()), g_flag);
    g_loop->RunAfter(std::chrono::seconds(3), foo4);
    g_flag = 3;
}

void foo4() {
    fmt::println("foo4(): tid={0}, Flag={1}", fmt::streamed(std::this_thread::get_id()), g_flag);
    g_loop->Quit();
}

int main() {
    fmt::println("main(): tid={0}, Flag={1}", fmt::streamed(std::this_thread::get_id()), g_flag);
    EventLoop loop;
    g_loop = EventLoop::GetCurrentThreadLoop();

    std::thread t(
        [&loop]() {
            fmt::println("another thread: tid={0}, Flag={1}", fmt::streamed(std::this_thread::get_id()), g_flag);
            loop.RunAfter(std::chrono::seconds(1), foo1);
            fmt::println("another thread: tid={0}, Flag={1}, success to add task", fmt::streamed(std::this_thread::get_id()), g_flag);
        }
    );
    t.join();
    loop.Loop();
    fmt::println("main(): tid={0}, Flag={1}", fmt::streamed(std::this_thread::get_id()), g_flag);
}