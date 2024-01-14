#if !defined(MUDUO_TIMER_H)
#define MUDUO_TIMER_H

#include <muduo/base/Logging.h>
#include <muduo/TimerQueue.h>
#include <muduo/TimerType.h>
#include <muduo/Channel.h>
#include <muduo/Timer.h>
#include <memory>
#include <memory>
#include <sys/timerfd.h>

namespace muduo {
using namespace detail;

class Timer {
    // noncopyable & nonmoveable
    Timer(const Timer&) = delete;
    Timer(Timer&&) = delete;

public:
    Timer(const TimePoint_t& time_point, const Interval_t& interval_us, const TimeoutCb_t& cb, const TimerId_t id)
        : cb_(cb)
        , expiration_(time_point)
        , interval_(interval_us)
        , id_(id)
        { }

    void Run() const { cb_(); }
    TimePoint_t ExpirationTime() const { return expiration_; }
    Interval_t Interval() const { return interval_; }
    TimerId_t GetId() const { return id_; }
    bool Repeat() const { return interval_ != Interval_t::zero(); }
    const TimeoutCb_t& GetPendingCallback() const { return cb_; }

private:
    TimeoutCb_t cb_;
    TimePoint_t expiration_;
    Interval_t interval_;
    TimerId_t id_;
};

/*************************************************************************************************/

/**
 * @brief 负责注册Timerfd，监控并处理Timer的超时事件(通知TimerQueue处理所有超时的Timer)
 */
#ifdef MUDUO_USE_MEMPOOL
class Watcher final : public base::detail::ManagedByMempoolAble<Watcher> {
#else
class Watcher {
#endif
    // non-copyable & non-moveable
    Watcher(const Watcher&) = delete;
    Watcher operator=(const Watcher&) = delete;

public:
    Watcher(TimerQueue* owner); 
    ~Watcher() noexcept;
    
    void HandleExpiredTimers();
    int FileDescriptor() const
    { return watcherChannel_->FileDescriptor(); }

    void SetTimerfd(struct itimerspec* old_t, struct itimerspec* new_t) const {
        int ret = ::timerfd_settime(FileDescriptor(), TFD_TIMER_ABSTIME, new_t, old_t);
        if (ret != 0) {
            LOG_SYSERR << "Failed to set timerfd";
            // wether need abort() ?
        }
    }

private:
    void ReadTimerfd() const;

private:
    TimerQueue* const owner_;   // 聚合
#ifdef MUDUO_USE_MEMPOOL
    std::unique_ptr<Channel, base::deleter_t<Channel>> watcherChannel_;
#else
    std::unique_ptr<Channel> watcherChannel_;
#endif
};

} // namespace muduo 



#endif // MUDUO_TIMER_H
