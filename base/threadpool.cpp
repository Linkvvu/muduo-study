#include <muduo/base/threadpool.h>
#include <muduo/base/Exception.h>
#include <algorithm>

namespace muduo {

threadpool::threadpool(const string& name)
    : name_(name)
    , running_(false)
    , mutex_()
    , notEmptyCV_(mutex_)
    , taskQueue_() 
    , threads_() {}

threadpool::~threadpool() {
    if (running_) {
        stop();
    }
}

void threadpool::start(std::size_t numThread) {
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThread);
    for (std::size_t i = 0; i < numThread; i++) {
        char id[20+1];
        snprintf(id, sizeof id, "%lu", i);
        threads_.emplace_back(new Thread(std::bind(&threadpool::runInThread, this), name_+string(id)));
        threads_[i]->start();
    }
}

void threadpool::stop() {
    running_ = false;
    {
        mutexLock_guard locker(mutex_);
        notEmptyCV_.notify_all();
    }
    std::for_each(threads_.begin(), threads_.end(), [](std::unique_ptr<muduo::Thread>& t) { t->join(); });
}

void threadpool::run(const threadpool::Task& func) {
    if (threads_.empty()) {
        func();
    } else {
        {
            mutexLock_guard locker(mutex_);
            taskQueue_.push_back(func);
        }
        notEmptyCV_.notify_one();
    }
}

/// @brief get a task in the task-queue
/// @return functor, 如果线程池状态为stop，且task-queue为空，则返回的std::function对象内部为空
threadpool::Task threadpool::take() {
    mutexLock_guard locker(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (taskQueue_.empty() && running_) {
        notEmptyCV_.wait();        
    }
    Task cur_task = nullptr;
    if (taskQueue_.empty() == false) {
        cur_task = taskQueue_.front();
        taskQueue_.pop_front();
    }
    return cur_task;
}

void threadpool::runInThread() {
    try {
        while (running_ || !taskQueue_.empty()) {
            Task task(take());
            if (task.operator bool())
                task();
        }
    } catch (const muduo::Exception& ec) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ec.what());
        fprintf(stderr, "stack trace: %s\n", ec.stackTrace());
        abort();
    } catch (const std::exception& ec) {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ec.what());
        abort();
    } catch (...) {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}

}


