#include <muduo/EventLoop.h>
#include <functional>
#include <sstream>
#include <thread>
#include <chrono>
#include <pthread.h>
#include <unistd.h>

using namespace muduo;
using namespace std::chrono;

int cnt = 0;
EventLoop* g_loop;

std::string getFormatedTime() {
    std::ostringstream oss;
    const auto now = std::chrono::system_clock::now();
    const time_t now_time = std::chrono::system_clock::to_time_t(now);
    oss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void printTid() {
    LOG_INFO << "pid = " << getpid() << ", tid = " << ::pthread_self();
    LOG_INFO << "now " << getFormatedTime();
}

void print(const char* msg) {
    LOG_INFO << "msg " << getFormatedTime() << ", " << msg;

    if (++cnt == 20) {
        g_loop->Quit();
    }
}

void cancel(TimerId_t timer) {
    g_loop->cancelTimer(timer);
    LOG_INFO << "cancelled at " << getFormatedTime();
}

int main() {
    LOG_INFO << "native thread-id=" << ::pthread_self();

    printTid();
    sleep(1);
    {
        auto loop = muduo::CreateEventLoop();
        g_loop = loop.get();

        print("main");
        loop->RunAfter(seconds(1), std::bind(print, "once1"));
        loop->RunAfter(seconds(1)+milliseconds(500), std::bind(print, "once1.5"));
        loop->RunAfter(seconds(2)+milliseconds(500), std::bind(print, "once2.5"));
        loop->RunAfter(seconds(3)+milliseconds(500), std::bind(print, "once3.5"));
        TimerId_t t45 = loop->RunAfter(seconds(4)+milliseconds(500), std::bind(print, "once4.5"));
        loop->RunAfter(seconds(4)+milliseconds(200), std::bind(cancel, t45));
        loop->RunAfter(seconds(4)+milliseconds(800), std::bind(cancel, t45));
        loop->RunEvery(seconds(2), std::bind(print, "every2"));
        TimerId_t t3 = loop->RunEvery(seconds(3), std::bind(print, "every3"));
        loop->RunAfter(seconds(9)+milliseconds(1), std::bind(cancel, t3));

        loop->Loop();
        print("main loop exits");
    }
}