#include <muduo/base/blockingQueue.h>
#include <muduo/base/countDownLatch.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

class Test {
public:
    Test(int numThreads)
        : latch_(numThreads)
        , threads_(numThreads) {
        for (int i = 0; i < numThreads; ++i) {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);
            threads_.push_back(new muduo::Thread(
                boost::bind(&Test::threadFunc, this), muduo::string(name)));
        }
        for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::start, _1));
    }

    void run(int times) {
        printf("waiting for count down latch\n");
        latch_.wait();
        printf("all threads started\n");
        for (int i = 0; i < times; ++i) {
            char buf[32];
            snprintf(buf, sizeof buf, "hello %d", i);
            queue_.put(buf);
            printf("tid=%d, put data = %s, size = %zd\n", muduo::currentThread::tid(), buf, queue_.size());
        }
    }

    void joinAll() {
        for (size_t i = 0; i < threads_.size(); ++i) {
            queue_.put("stop");
        }

        for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::join, _1));
    }

private:

    void threadFunc() {
        printf("tid=%d, %s started\n",
            muduo::currentThread::tid(),
            muduo::currentThread::name());

        latch_.countdown();
        bool running = true;
        while (running) {
            std::string data(queue_.take());
            printf("tid=%d, get data = %s, size = %zd\n", muduo::currentThread::tid(), data.c_str(), queue_.size());
            running = (data != "stop");
        }

        printf("tid=%d, %s stopped\n",
            muduo::currentThread::tid(),
            muduo::currentThread::name());
    }

    muduo::blockingQueue<std::string> queue_;
    muduo::countDown_latch latch_;
    boost::ptr_vector<muduo::Thread> threads_;
};

int main() {
    printf("pid=%d, tid=%d\n", ::getpid(), muduo::currentThread::tid());
    Test t(5);
    t.run(100);
    t.joinAll();

    printf("number of created threads %d\n", muduo::Thread::numCreated());
}