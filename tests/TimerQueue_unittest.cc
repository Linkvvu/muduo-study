#include <EventLoop.h>
#include <fmt/ostream.h>
#include <fmt/chrono.h>
#include <functional>
#include <pthread.h>
#include <unistd.h>
#include <thread>

using namespace muduo;
using namespace std::chrono;

int cnt = 0;
EventLoop* g_loop;

void printTid() {
    fmt::print("pid = {0}, tid = {1}\n", getpid(), fmt::streamed(std::this_thread::get_id())); 
    fmt::println("now {:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::now());  
}

void print(const char* msg) {
    fmt::println("msg {:%Y-%m-%d %H:%M:%S}, {}", system_clock::now(), msg);

    if (++cnt == 20) {
        g_loop->Quit();
    }
}

void cancel(TimerId_t timer) {
    g_loop->cancelTimer(timer);
    fmt::println("cancelled at {:%Y-%m-%d %H:%M:%S}", std::chrono::system_clock::now()); 
}

int main() {
    fmt::println("native thread-id={}", ::pthread_self());

    printTid();
    sleep(1);
    {
        EventLoop loop;
        g_loop = &loop;

        print("main");
        loop.RunAfter(seconds(1), std::bind(print, "once1"));
        loop.RunAfter(seconds(1)+milliseconds(500), std::bind(print, "once1.5"));
        loop.RunAfter(seconds(2)+milliseconds(500), std::bind(print, "once2.5"));
        loop.RunAfter(seconds(3)+milliseconds(500), std::bind(print, "once3.5"));
        TimerId_t t45 = loop.RunAfter(seconds(4)+milliseconds(500), std::bind(print, "once4.5"));
        loop.RunAfter(seconds(4)+milliseconds(200), std::bind(cancel, t45));
        loop.RunAfter(seconds(4)+milliseconds(800), std::bind(cancel, t45));
        loop.RunEvery(seconds(2), std::bind(print, "every2"));
        TimerId_t t3 = loop.RunEvery(seconds(3), std::bind(print, "every3"));
        loop.RunAfter(seconds(9)+milliseconds(1), std::bind(cancel, t3));

        loop.Loop();
        print("main loop exits");
    }
}