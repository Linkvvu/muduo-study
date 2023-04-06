#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include "muduo/base/countDownLatch.h"
#include "muduo/base/currentThread.h"
#include "muduo/base/Thread.h"

using namespace muduo;

class Test {
public:
    Test(int numThreads)
        : latch_(1),
        threads_(numThreads) {
        for (int i = 0; i < numThreads; ++i) {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);
            threads_[i].reset(new Thread(std::bind(&Test::threadFunc, this), name));
        }
        std::for_each(threads_.begin(), threads_.end(), [](std::unique_ptr<Thread>& t) { t->start(); });
    }

    void run() {
        latch_.countdown();
    }

    void joinAll() {
        std::for_each(threads_.begin(), threads_.end(), [](std::unique_ptr<Thread>& t) { t->join(); });
    }

private:
    void threadFunc() {
        latch_.wait();
        printf("tid=%d, %s started\n", currentThread::tid(), currentThread::name());

        printf("tid=%d, %s stopped\n", currentThread::tid(), currentThread::name());
    }
    
private:
    countDown_latch latch_;
    std::vector<std::unique_ptr<Thread>> threads_;
};

int main() {
    printf("pid=%d, tid=%d\n", ::getpid(), currentThread::tid());
    Test t(3);
    sleep(3);
    printf("pid=%d, tid=%d %s running ...\n", ::getpid(), currentThread::tid(), currentThread::name());
    t.run();
    t.joinAll();
    printf("number of created threads %d\n", Thread::numCreated());
}