/**
 * 该测试程序通过countdown_latch实现：主线程等待所有的子线程初始化完成后才开始运行
*/

#include <muduo/base/countDownLatch.h>
#include <muduo/base/currentThread.h>
#include <muduo/base/Thread.h>
#include <algorithm>
#include <vector>
#include <memory>

using namespace muduo;

class Test {
public:
    Test(int threadNum)
        : latch_(threadNum)
        , threads_(threadNum) {
        for (int i = 0; i < threadNum; ++i) {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);
            threads_[i].reset(new muduo::Thread([this]() { this->threadFunc(); }, name));
        }
            std::for_each(threads_.begin(), threads_.end(), [](std::unique_ptr<muduo::Thread>& t) { t->start(); });
        }

    void wait() {
        latch_.wait();
    }

    void joinAll() {
        std::for_each(threads_.begin(), threads_.end(), [this](std::unique_ptr<muduo::Thread>& t) {
            t->join();
        });
    }

private:
void threadFunc() {
    sleep(3);
    latch_.countdown();
    printf("tid=%d, %s started\n", currentThread::tid(), currentThread::name());

    printf("tid=%d, %s stopped\n", currentThread::tid(), currentThread::name());
}

private:
    muduo::countDown_latch latch_;
    std::vector<std::unique_ptr<muduo::Thread>> threads_;
};

int main() {
    printf("pid=%d, tid=%d\n", ::getpid(), currentThread::tid());
    Test t(4);
    t.wait();
    printf("pid=%d, tid=%d %s running ...\n", ::getpid(), currentThread::tid(), currentThread::name());
    t.joinAll();
    printf("number of created threads %d\n", Thread::numCreated());
}