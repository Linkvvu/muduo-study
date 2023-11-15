#include <muduo/EventLoop.h>
#include <muduo/EventLoopThread.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace muduo;

void print(EventLoop* p = NULL) {
    std::cout << "print: pid = " << std::this_thread::get_id()
        << ", loop = " << p << std::endl;
}

void quit(EventLoop* p) {
    print(p);
    p->Quit();
}

int main() {
    print();

    {
        EventLoopThread thr1;  // never start
    }
    std::cout << "=========================" << std::endl;
    {
        // dtor calls Quit()
        EventLoopThread thr2;
        EventLoop* loop = thr2.Run();
        loop->RunInEventLoop(std::bind(print, loop));
        std::this_thread::sleep_for(std::chrono::microseconds(1000)*500);
    }
    std::cout << "=========================" << std::endl;
    {
        // Quit() before dtor
        EventLoopThread thr3;
        EventLoop* loop = thr3.Run();
        loop->RunInEventLoop(std::bind(quit, loop));
        std::this_thread::sleep_for(std::chrono::microseconds(1000)*500);
    }
    std::cout << "=========================" << std::endl;
    {
        // When thread-func running initCallback, EventLoopThread is destroyed
        EventLoopThread thr3([](EventLoop*) { 
            std::cout << "now sleep 3s" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3)); 
        });
        std::thread t(std::bind(&EventLoopThread::Run, &thr3));
        t.detach();
        std::this_thread::sleep_for(std::chrono::microseconds(1000)*500);
    }
}

