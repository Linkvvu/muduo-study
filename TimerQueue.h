#if !defined(MUDUO_TIMER_QUEUE_H)
#define MUDUO_TIMER_QUEUE_H

#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <muduo/TimerType.h>
#include <chrono>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

namespace muduo {
using namespace detail;

class EventLoop;    // forward declaration
class Watcher;      // forward declaration
class Timer;        // forward declaration

#ifdef MUDUO_USE_MEMPOOL
class TimerQueue final : public base::detail::ManagedByMempoolAble<TimerQueue> {
#else
class TimerQueue {
#endif

    // non-copyable & non-moveable
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue operator=(const TimerQueue) = delete;
    
    class TimerMinHeap;

public:
    TimerQueue(EventLoop* owner);
    ~TimerQueue() noexcept;

    /** 
     * thread-safe
    */
    TimerId_t AddTimer(const TimePoint_t& when, const Interval_t& interval, const TimeoutCb_t& cb);
    void CancelTimer(const TimerId_t id);
    
    /**
     * return the loop instance whose own the current channel
    */
    EventLoop* Owner() { return owner_; }
    const EventLoop* Owner() const { return owner_; }

    void HandleExpiredTimers();

private:
    void AddTimerInLoop(std::unique_ptr<Timer>& t_p);
    void CancelTimerInLoop(const TimerId_t id);
    void ResetTimerfd();
    using ExpiredTimerList = std::vector<std::unique_ptr<Timer>>; 
    ExpiredTimerList GetExpiredTimers();

private:
    EventLoop* const owner_;
#ifdef MUDUO_USE_MEMPOOL
    std::unique_ptr<Watcher, base::deleter_t<Watcher>> watcher_;
    std::unique_ptr<TimerMinHeap> heap_;
#else
    std::unique_ptr<Watcher> watcher_;
    std::unique_ptr<TimerMinHeap> heap_;
#endif
    TimerId_t nextTimerId_;
    TimePoint_t latestTime_;
};

} // namespace muduo 

#endif // MUDUO_TIMER_QUEUE_H
