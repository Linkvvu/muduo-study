#if !defined(MUDUO_BASE_MUTEX_H)
#define MUDUO_BASE_MUTEX_H

#include <muduo/base/currentThread.h>
#include <boost/noncopyable.hpp>

#include <pthread.h>
#include <cassert>
namespace muduo {

class mutexLock : private boost::noncopyable {
public:
    mutexLock() : holdThread_Id_(0) {
        auto ret = pthread_mutex_init(&mutex_, nullptr);
        assert(ret == 0); (void)ret;
    }

    ~mutexLock() {
        assert(holdThread_Id_ == 0);
        int ret = pthread_mutex_destroy(&mutex_);
        assert(ret == 0); (void)ret;
    }

    bool isLockedByCurrentThread() {
        return holdThread_Id_ == currentThread::tid();
    }

    void assertLockedByThisThread() {
        assert(isLockedByCurrentThread());
    }

    void lock() {
        pthread_mutex_lock(&mutex_);
        holdThread_Id_ = currentThread::tid();
    }

    void unlock() {
        holdThread_Id_ = 0;
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t* getLockHandle() { /* non const */
        return &mutex_;
    }

private:
    pid_t holdThread_Id_;
    pthread_mutex_t mutex_;        
};

class mutexLock_guard : private boost::noncopyable {
public:
    explicit mutexLock_guard(mutexLock& mutex) : mutex_(mutex) {
        mutex_.lock();
    }

    ~mutexLock_guard() {
        mutex_.unlock();
    }
    
private:
    mutexLock& mutex_; // 聚合关系：不负责聚合对象的生命周期
};


} // namespace muduo 


#endif // MUDUO_BASE_MUTEX_H
