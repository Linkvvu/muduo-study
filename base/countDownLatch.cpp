#include "countDownLatch.h"

// 注意：countDownLatch的实现应该保证thread-safe，因为其被应用于多线程环境下进行线程同步

muduo::countDown_latch::countDown_latch(const int count)
    : mutex_()
    , cv_(mutex_)
    , count_(count) {}

void muduo::countDown_latch::wait() {
    mutexLock_guard locker(mutex_);
    while (count_ > 0) {
        cv_.wait();
    }
}

void muduo::countDown_latch::countdown() {
    mutexLock_guard locker(mutex_);
    count_--;
    if (count_ == 0) {
        cv_.notify_all();
    }
}

int muduo::countDown_latch::getCount() const {
    mutexLock_guard locker(mutex_);
    return count_;
}

