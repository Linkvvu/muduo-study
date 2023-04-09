#include <muduo/base/bounded_blockingQueue.h>
#include <muduo/base/countDownLatch.h>
#include <muduo/base/Thread.h>

#include <functional>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>

class Test {
public:
    Test(int numThreads) 
        : threads_(numThreads)
        , latch_(numThreads)
        , queue_(15) {
            for (int i = 0; i < numThreads; i++) {
                char name[32];
                snprintf(name, sizeof name, "work thread %d", i);
                threads_[i].reset(new muduo::Thread([this]() { this->threadFunc(); }, name));
            }
            std::for_each(threads_.begin(), threads_.end(), [](std::unique_ptr<muduo::Thread>& t) { t->start(); });
        }

    void run(int times) {
        printf("waiting for count down latch\n");
        latch_.wait();
        printf("all threads started\n");
        for (int i = 0; i < times; i++) {
            char data[32];
            snprintf(data, sizeof data, "hello %d", i);
            queue_.put(data);
            printf("thread: %s, put data = %s, size = %zd\n", muduo::currentThread::name(), data, queue_.size());
        }
    }   

    void joinAll() {
        for (std::size_t i = 0; i < threads_.size(); i++) {
            queue_.put("stop");
        }
        std::for_each(threads_.begin(), threads_.end(), [](std::unique_ptr<muduo::Thread>& t) { t->join(); });
    }

private:
    void threadFunc() {
        printf("tid=%d %s started\n", muduo::currentThread::tid(), muduo::currentThread::name());
        latch_.countdown();
        bool running = true;
        while (running) {
            std::string front_data(queue_.take());
            printf("thread: %s, get data: %s, size = %zd\n",
                muduo::currentThread::name(),
                front_data.c_str(),
                queue_.size()
            );
            running = (front_data != "stop");
        }
        printf("tid=%d, %s stopped\n", muduo::currentThread::tid(), muduo::currentThread::name());
    }

private:
    std::vector<std::unique_ptr<muduo::Thread>> threads_;
    muduo::countDown_latch latch_;
    muduo::bounded_blockingQueue<std::string> queue_;
};

int main() {
    printf("pid=%d, tid=%d\n", ::getpid(), muduo::currentThread::tid());
    Test t(5);
    t.run(100);
    t.joinAll();

    printf("number of created threads: %d\n", muduo::Thread::numCreated());
}