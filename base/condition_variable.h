#if !defined(MUDUO_BASE_CV_H)
#define MUDUO_BASE_CV_H

#include <muduo/base/mutex.h>
#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <errno.h>
#include <time.h>

namespace muduo {
    
class condition_variable : boost::noncopyable {
public:
    explicit condition_variable(mutexLock& mutex) : mutex_(mutex) {
        auto ret = pthread_cond_init(&cv_, nullptr);
        assert(ret == 0); (void)ret;
    }

    ~condition_variable() {
        auto ret = pthread_cond_destroy(&cv_);
        assert(ret == 0); (void)ret;
    }

    void wait() {
        pthread_cond_wait(&cv_, mutex_.getLockHandle());
    }

    void notify_one() {
        pthread_cond_signal(&cv_);
    }

    void notify_all() {
        pthread_cond_broadcast(&cv_);
    }   

    // returns true if time out, false otherwise.
    bool wait_for(const int seconds) {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += seconds;
        return ETIMEDOUT == pthread_cond_timedwait(&cv_, mutex_.getLockHandle(), &abstime);
    }

private:
    mutexLock& mutex_; // 聚合关系：本类对象不负责聚合对象的生命周期
    pthread_cond_t cv_;
};

} // namespace muduo 

#endif // MUDUO_BASE_CV_H
