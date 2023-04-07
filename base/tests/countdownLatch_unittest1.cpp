/**
 * 该测试程序通过countdown_latch实现：所有的子线程等待主线程初始化完成后才开始运行
*/
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
    // note: 由于muduo::Thread为noncopyable类型，其没有拷贝和移动构造函数，故不能直接存放在vector中
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