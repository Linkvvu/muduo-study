#if !defined(MUDUO_EVENTLOOP_THREAD_H)
#define MUDUO_EVENTLOOP_THREAD_H
#include <Callbacks.h>
#include <mutex>
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>

namespace muduo {

class EventLoopThread {
    // non-copyable & non-moveable
    EventLoopThread(const EventLoopThread&) = delete;
    EventLoopThread operator=(const EventLoopThread&) = delete;
public:
    EventLoopThread(const IoThreadInitCb_t& cb = nullptr, const std::string& name = std::string());
    ~EventLoopThread() noexcept;

    /* @note Only Can be called once */
    EventLoop* Run();

private:
    void ThreadFunc();

private:
    EventLoop* loop_;   // EventLoop instance of IO-thread
    std::string name_;
    IoThreadInitCb_t initCb_;
    std::thread IoThread_;              // current IO-thread
    bool isExit_;                       // The state dicates whether IO thread exits 
    /* for sync operations on loop_ */
    std::mutex mtx_;
    std::condition_variable cv_;
};


} // namespace muduo 


#endif // MUDUO_EVENTLOOP_THREAD_H
