#if !defined(MUDUO_EVENTLOOP_THREAD)
#define MUDUO_EVENTLOOP_THREAD

#include <Callbacks.h>
#include <memory>
#include <vector>
#include <atomic>

namespace muduo {
class EventLoopThread;  // forward declaration
class EventLoopThreadPool {
    // non-copyable & non-moveable
    EventLoopThreadPool(const EventLoopThreadPool&) = delete;
    EventLoopThreadPool operator=(const EventLoopThreadPool&) = delete;
public:
    EventLoopThreadPool(EventLoop* base_loop, const std::string& name);
    ~EventLoopThreadPool() noexcept;

    /* @note: Pool size is 0 by default */
    void SetPoolSize(const size_t s)
    { poolSize_ = s; }

    /* @note: init-callback is NULL by default */
    void SetThreadInitCallback(const IoThreadInitCb_t& cb)
    { initCb_ = cb; }

    bool IsStarted() const
    { return started_; }

    std::string Name() const
    { return name_; }

    void BuildAndRun();
    EventLoop* GetNextLoop() const;

private:
    EventLoop* const baseLoop_;
    const std::string& name_;
    IoThreadInitCb_t initCb_ {nullptr};
    std::size_t poolSize_ {0};
    mutable std::size_t nextLoopIdx_ {0};
    std::atomic_bool started_ {false};
    std::vector<std::unique_ptr<EventLoopThread>> threadPool_;
    std::vector<EventLoop*> loops_;
};


} // namespace muduo 

#endif // MUDUO_EVENTLOOP_THREAD
