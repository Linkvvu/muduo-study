// This is an internal header file, you should not include this.
#if !defined(MUDUO_NET_TIMERQUEUE_H)
#define MUDUO_NET_TIMERQUEUE_H
#include <muduo/base/timeStamp.h>
#include <muduo/net/callBacks.h>
#include <muduo/net/channel.h>
#include <atomic>
#include <vector>
#include <set>
#include <unordered_set>

namespace muduo{
namespace net {

class timer;        // forward declaration 
class event_loop;   // forward declaration
class timer_id;     // forward declaration

class timer_queue : private boost::noncopyable {
    using fd_t = int;
    using entry = std::pair<TimeStamp, timer*>; // 按时间戳排序，时间戳相等时再按指针指向的地址大小排序
    using timerList_t = std::set<entry>;
    using activeTimerSet_t = std::unordered_set<timer*>;  // O(1)

public:
    explicit timer_queue(event_loop* const loop);
    ~timer_queue();
    
    ///
    /// Schedules the callback to be run at given time,
    /// repeats if @c interval > 0.0.
    ///
    /// Must be thread safe. Usually be called from other threads.    
    /// @brief set a timer to invoke target task
    timer_id add_timer(TimeStamp expir, double interval, timerCallback_t callBack);
    
    void cancel(timer_id t_id);

private:
    void add_timer_inLoop(timer* t);
    void cancel_timer_inLoop(timer* t);
    bool insert(timer* t);
    void handle_expiredTimers();
    std::vector<entry> get_expired_timers(TimeStamp pre);
    // 重新设置定时器的下次过期时间
    void reset_timer() const;
    // 将"repeat timer"重新插入集合
    void reset_repeat_timers(const std::vector<entry>& list, TimeStamp now);
    void remedial_work(const std::vector<entry>& list, TimeStamp now) {
        reset_repeat_timers(list, now);
        reset_timer();
    };

private:
    event_loop* loop_;  // 所属event_loop
    fd_t timerfd_;
    channel timerChannel_;
    timerList_t timers_;
    activeTimerSet_t activeTimers_;
    activeTimerSet_t cancelingTimers_;
    std::atomic<bool> handlingExpiredTimer_;
};
    
} // namespace net     
} // namespace muduo

#endif // MUDUO_NET_TIMERQUEUE_H
