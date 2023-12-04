#if !defined(MUDUO_BASE_THREAD_POOL_H)
#define MUDUO_BASE_THREAD_POOL_H
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <functional>
#include <condition_variable>

namespace muduo {
class ThreadPool
{
private:
    // non-copyable & non-moveable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
public:
    using Task_t = std::function<void()>;

    explicit ThreadPool(const std::string& pool_name = std::string("thread-pool"))
        : name_(pool_name)
        { }

    ~ThreadPool();

    void SetThreadInitCallback(const Task_t& func)
    { threadInitCb_ = func; }

    /// Build and run the pool 
    /// @note Must be called only once
    void Start(int thread_num);

    /// Stop the ThreadPool
    /// @note: Must only be called once
    void Stop();

    /// Add a task to run in thread-pool
    /// could block if maxQueueSize > 0 and the queue is full
    void Run(Task_t task);

    size_t TaskQueueSize() const
    {
        std::lock_guard<std::mutex> guard(mutex_);
        return taskQueue_.size();
    }

    void SetMaxQueueSize(size_t n) 
    { maxQueueSize_ = n; }

private:
    void ThreadFunc();
    Task_t Take();
    bool IsFull() const;

private:
    std::string name_;
    std::atomic_bool running_ {false};
    Task_t threadInitCb_ {nullptr};
    std::vector<std::unique_ptr<std::thread>> pool_ {};

    std::deque<Task_t> taskQueue_ {};
    size_t maxQueueSize_ {0};
    mutable std::mutex mutex_ {};
    std::condition_variable notEmptyCv_ {};
    std::condition_variable notFullCv_ {};
};

} // namespace muduo 

#endif // MUDUO_BASE_THREAD_POOL_H
