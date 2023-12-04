#include <muduo/base/ThreadPool.h>
#include <muduo/base/Logging.h>
#include <cassert>

using namespace muduo;

muduo::ThreadPool::~ThreadPool() {
    if (running_) {
        Stop();
    }
}

void ThreadPool::Start(int thread_num) {
    assert(pool_.empty());
    assert(!running_);

    running_ = true;
    
    pool_.reserve(thread_num);
    for (int i = 0; i < thread_num; i++) {
        pool_.emplace_back(std::make_unique<std::thread>(
            [this]() {
                this->ThreadFunc();
            }
        ));
    }

    if (thread_num == 0 && threadInitCb_) {
        threadInitCb_();
    }
}

ThreadPool::Task_t ThreadPool::Take() {
    std::unique_lock<std::mutex> guard(mutex_);
    while (taskQueue_.empty() && running_) {
        notEmptyCv_.wait(guard);
    }

    Task_t task;
    if (!taskQueue_.empty()) {
        task = taskQueue_.front();
        taskQueue_.pop_front();
        
        // notify not-full cv if the task-queue is full
        // consider it unlikely that the queue is full, so check @c taskQueue_.size() == maxQueueSize_ - 1
        if (maxQueueSize_ > 0 && taskQueue_.size() == maxQueueSize_ - 1) { 
            notFullCv_.notify_one();
        }
    }

    return task;
}

void ThreadPool::ThreadFunc() {
    if (threadInitCb_) {
        threadInitCb_();
    }

    while (running_) {
        Task_t t = Take();
        if (t) {
            t();
        }
    }
}

bool ThreadPool::IsFull() const {
    // assert that mutex is locked currently
    assert(mutex_.try_lock() == false);
    return maxQueueSize_ > 0 && taskQueue_.size() == maxQueueSize_;
}

void ThreadPool::Run(Task_t task) {
    if (pool_.empty()) {
        task();
    } else {
        std::unique_lock<std::mutex> guard(mutex_);
        if (IsFull() && running_) {
            notFullCv_.wait(guard);
        }

        if (!running_) {
            LOG_WARN << "Failed to add task in ThreadPool::Run, cause " << name_ << " is inactive,"
                << " detail: &Task_t=" << &task;
            return;
        }

        assert(!IsFull());
        taskQueue_.push_back(std::move(task));
        notEmptyCv_.notify_one();
    }
}

void ThreadPool::Stop() {
    assert(running_);
    {   /* notify all threads to exit */
        std::lock_guard<std::mutex> guard(mutex_);
        running_ = false;
        notEmptyCv_.notify_all();
        notFullCv_.notify_all();
    }
    for (const auto& thread : pool_) {
        thread->join();
    }
}
