#include <muduo/base/threadpool.h>
#include <muduo/base/countDownLatch.h>

void print() {
    printf("tid=%d, thread: %s\n", muduo::currentThread::tid(), muduo::currentThread::name());
}

void printString(const std::string& str) {
    printf("tid=%d, string: %s\n", muduo::currentThread::tid(), str.c_str());
}

int main() {
    using namespace muduo;
    threadpool pool("mainThreadPool");
    pool.start(5);

    pool.run(print);
    pool.run(print);
    for (int i = 0; i < 100; i++) {
        char buf[17];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.run([buf]() { printString(buf); });
    }

    muduo::countDown_latch latch(1);
    pool.run([&latch]() { latch.countdown(); });
    latch.wait();
    
    pool.stop();
    return 0;
}