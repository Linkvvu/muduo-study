#include <muduo/net/eventLoop.h>
//#include <muduo/net/EventLoopThread.h>
#include <muduo/base/Thread.h>
#include <muduo/net/timerId.h>

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

int cnt = 0;
event_loop* g_loop;

void printTid() {
    printf("pid = %d, tid = %d\n", getpid(), currentThread::tid());
    printf("now %s\n", TimeStamp::now().toString().c_str());
}

void print(const char* msg) {
    printf("msg %s %s\n", TimeStamp::now().toString().c_str(), msg);
    if (++cnt == 20) {
        g_loop->quit();
    }
}

void cancel(timer_id timer) {
    g_loop->cancel(timer);
    printf("cancelled at %s\n", TimeStamp::now().toString().c_str());
}

int main() {
    printTid();
    sleep(1);
    {
        event_loop loop;
        g_loop = &loop;

        print("main");
        loop.run_after(1, boost::bind(print, "once1"));
        loop.run_after(1.5, boost::bind(print, "once1.5"));
        loop.run_after(2.5, boost::bind(print, "once2.5"));
        loop.run_after(3.5, boost::bind(print, "once3.5"));
        timer_id t45 = loop.run_after(4.5, boost::bind(print, "once4.5"));
        loop.run_after(4.2, boost::bind(cancel, t45));
        loop.run_after(4.8, boost::bind(cancel, t45));
        loop.run_every(2, boost::bind(print, "every2"));
        timer_id t3 = loop.run_every(3, boost::bind(print, "every3"));
        loop.run_after(9.001, boost::bind(cancel, t3));

        loop.loop();
        print("main loop exits");
    }
    // sleep(1);
    // {
    //   EventLoopThread loopThread;
    //   EventLoop* loop = loopThread.startLoop();
    //   loop->runAfter(2, printTid);
    //   sleep(3);
    //   print("thread loop exits");
    // }
}