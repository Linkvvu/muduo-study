#if !defined(MUDUO_EVENTLOOP_THREAD)
#define MUDUO_EVENTLOOP_THREAD

#include <muduo/base/allocator/Allocatable.h>
#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <muduo/Callbacks.h>
#include <string>
#include <memory>
#include <vector>
#include <atomic>

namespace muduo {
class EventLoopThread;  // forward declaration

#ifdef MUDUO_USE_MEMPOOL
class EventLoopThreadPool : public base::detail::Allocatable {
#else
class EventLoopThreadPool {
#endif
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
    void SetThreadInitCallback(const IoThreadInitCallback_t& cb)
    { initCb_ = cb; }

    bool IsStarted() const
    { return started_; }

    const std::string& Name() const
    { return name_; }

    void BuildAndRun();
    EventLoop* GetNextLoop() const;

private:
    EventLoop* const baseLoop_;
    std::string name_;
    IoThreadInitCallback_t initCb_ {nullptr};
    std::size_t poolSize_ {0};
    mutable std::size_t nextLoopIdx_ {0};
    std::atomic_bool started_ {false};
#ifdef MUDUO_USE_MEMPOOL
    std::vector<std::unique_ptr<EventLoopThread>, base::allocator<std::unique_ptr<EventLoopThread>>> threadPool_;
    std::vector<EventLoop*, base::allocator<EventLoop*>> loops_;
#else
    std::vector<std::unique_ptr<EventLoopThread>> threadPool_;
    std::vector<EventLoop*> loops_;
#endif
};


} // namespace muduo 

#endif // MUDUO_EVENTLOOP_THREAD
