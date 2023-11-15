#if !defined(MUDUO_BASE_ASYNC_LOGGING_H)
#define MUDUO_BASE_ASYNC_LOGGING_H
#include <muduo/base/LogStream.h>
#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <mutex>
#include <condition_variable>

namespace muduo {

class AsyncLogger {
    using Buffer = base::detail::FixedBuffer<base::detail::kLargeBuffer>;
    using BufferPtr = std::unique_ptr<Buffer>;
    using seconds = std::chrono::seconds;

public:
    AsyncLogger(const std::string& basename, off_t roll_size, seconds flush_interval = seconds(3));
    ~AsyncLogger();
    
    void Start() {
        bool expected = false;
        bool swapped = running_.compare_exchange_strong(expected, true);
        if (swapped) {
            /* start back-thread */
            backThread_.reset(
                new std::thread(&AsyncLogger::ThreadFunc, this)
            );
        }
    }

    void Append(const char* logline, size_t size);

    void Stop() {
        bool expected = true;
        if (running_.compare_exchange_strong(expected, false)) {
            nonEmptyCV_.notify_one();   // It`s okey, becasue only has one back-thread   
            backThread_->join();
        }
    }

private:
    void ThreadFunc();

private:
    std::atomic_bool running_ {false};
    std::string basename_;
    off_t rollSize_;
    seconds flushInterval_;

    std::vector<BufferPtr> buffers_ {};
    BufferPtr currentBuffer_;
    BufferPtr nextBuffer_;

    std::unique_ptr<std::thread> backThread_ {nullptr};
    std::mutex mutex_;
    std::condition_variable nonEmptyCV_;
};

} // namespace muduo 

#endif // MUDUO_BASE_ASYNC_LOGGING_H
