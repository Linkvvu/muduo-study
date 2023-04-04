#if !defined(MUDUO_BASE_THREAD_H)
#define MUDUO_BASE_THREAD_H

#include "muduo/base/Types.h"
#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <atomic>
#include <functional>

namespace muduo {
    class Thread : private boost::noncopyable {
    public:
        using ThreadFunc = std::function<void()>;

        explicit Thread(const ThreadFunc&, const string& name = string());
        ~Thread();

        void start();
        int join(); // return pthread_join()
        
        bool started() const { return started_; }
        pid_t tid() const { return tid_; }
        const string& name() const { return name_; }
        static int numCreated() { return createdCount; }

    private:
        static void* startThread(void* args);
        void runInThread();

    private:
        static std::atomic<int> createdCount;

    private:
        bool started_;
        bool joined_;
        pthread_t pthreadId_;
        pid_t tid_;
        ThreadFunc func_;
        string name_;
    };
} // namespace muduo

#endif // MUDUO_BASE_THREAD_H
