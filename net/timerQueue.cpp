#include <muduo/base/logging.h>
#include <muduo/net/timerQueue.h>
#include <muduo/net/eventLoop.h>
#include <muduo/net/timerId.h>
#include <muduo/net/timer.h>
#include <sys/timerfd.h>
#include <cassert>

using namespace muduo;
using namespace net;

namespace muduo::net::detail {

int create_timerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);   // 单调时钟
    if (timerfd == -1) {
        LOG_SYSFATAL << "failed in create_timerfd";        
    }
    return timerfd;
}

struct timespec how_much_time_from_now(TimeStamp when) {
    // Get relative time
    int64_t microseconds = when.microSecondsSinceEpoch() - TimeStamp::now().microSecondsSinceEpoch();
    struct timespec ret = {0};
    ret.tv_sec = static_cast<time_t>(microseconds) / TimeStamp::kMicroSecondsPerSecond;
    ret.tv_nsec = static_cast<long>((microseconds % TimeStamp::kMicroSecondsPerSecond) * 1000);
    if (ret.tv_sec < 0 || ret.tv_nsec < 0) {
        // 存在以下情况：
        //   当前timerQueue::timerfd过期，poll回调其处理函数"timer_queue::handle_expiredTimers"
        //   处理函数的实现是获取当前时间，然后获取所有小于等于当前时间的timer，并执行它们的回调
        //   执行它们回调也需要耗时，若在执行的过程中原集合中又有定时器(the first must be front timer in set)过期
        //   则当处理完所有过期的timer后，会执行"timer_queue::reset_timer"，此时该函数会传递一个比当前时间小的时间戳
        //   故会发生 timespec 为负数的情况！
        // how:
        //   当发生上述情况时，我们指定定时器下次过期是在1us后，这样 timerQueue 方可再次正常运行
        LOG_INFO << "in net::detail::reset_timerfd()" << " new expiration is a invalid value";
        ret.tv_sec = 0;
        ret.tv_nsec = 1000;
    }
    return ret;
}

void reset_timerfd(int timerfd, TimeStamp expiration) {
    struct itimerspec newvalue = {0};
    newvalue.it_value = how_much_time_from_now(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newvalue, nullptr);  // flag=0：定时器以相对时间计时
    if (ret != 0) {
        LOG_SYSERR << "failed in reset timerfd time";
    }
}

void read_timerData(int timerfd, TimeStamp when) {
    uint64_t count = 0; // containing the number of expirations that have occurred
    int n = ::read(timerfd, &count, sizeof count);
    if (n != sizeof count) {
        LOG_SYSERR << "timer_queue::handle_expiredTimers() reads " << n << " bytes instead of 8";
    } else {
        LOG_TRACE << "timer_queue::handle_expiredTimers() " << count << " at " << when.toString();
    }
}

} // namespace net::detail 

timer_queue::timer_queue(event_loop* const loop)
    : loop_(loop)
    , timerfd_(net::detail::create_timerfd())
    , timerChannel_(loop, timerfd_)
    , timers_()
    , activeTimers_() {
        timerChannel_.setReadCallback(std::bind(&timer_queue::handle_expiredTimers, this));
        timerChannel_.enableReading();
    }

timer_queue::~timer_queue() {
    timerChannel_.disableAllEvents();
    timerChannel_.remove();
    ::close(timerfd_);
    // do not remove channel, because we're in EventLoop::dtor(), event_loop don`t work;
    for (const entry& cur_pair : timers_) {
        delete cur_pair.second;
    }
}

timer_id timer_queue::add_timer(TimeStamp expir, double interval, timerCallback_t callBack) {
    timer* instance = new timer(expir, interval, callBack);
    loop_->run_in_eventLoop([this, instance]() { this->add_timer_inLoop(instance); });
    return timer_id(instance, instance->sequence());
}

void timer_queue::add_timer_inLoop(timer* t) {
    loop_->assert_loop_in_hold_thread();    // ensure thread safe
    bool earliestChanged = insert(t);
    if (earliestChanged) {
        detail::reset_timerfd(timerfd_, t->expiration());
    }
}

bool timer_queue::insert(timer* t) {
    loop_->assert_loop_in_hold_thread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;
    TimeStamp when = t->expiration();
    // 如果集合为空 || 新定时器的过期时间早于第一个定时器的过期时间
    if (timers_.begin() == timers_.end() || when < (*timers_.begin()).first) {
        earliestChanged = true;
    }

    {
        std::pair<timerList_t::iterator, bool> ret = timers_.emplace(when, t);
        assert(ret.second); (void)ret;
    }

    {
        auto ret = activeTimers_.emplace(t);
        assert(ret.second); (void)ret;
    }
    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}

void timer_queue::handle_expiredTimers() {
    loop_->assert_loop_in_hold_thread();
    TimeStamp now = TimeStamp::now(); // 截至到当前时间的定时器全部记为过期(包括当前时间)
    detail::read_timerData(timerfd_, now);
    std::vector<entry> expired_timers = get_expired_timers(now);
    
    {   // 调用各定时器绑定的callbacks
        cancelingTimers_.clear();
        handlingExpiredTimer_ = true;
        for (const entry& cur_entry : expired_timers) {
            cur_entry.second->run();
        }
        handlingExpiredTimer_ = false;
    }

    remedial_work(expired_timers, now);
}

std::vector<timer_queue::entry> timer_queue::get_expired_timers(TimeStamp limit) {
    assert(timers_.size() == activeTimers_.size());
    
    std::vector<entry> result;
    entry sentry(limit, reinterpret_cast<timer*>(UINTPTR_MAX));
    // 获取第一个大于sentry原始的迭代器
    // 将sentry.second的值设为 UINTPTR_MAX 是为了使得当时间戳相等时, 避免timer*的大小小于容器中的元素进而导致没有完全返回已过期的定时器的可能性
    timerList_t::iterator end = timers_.upper_bound(sentry);
    assert(end == timers_.end() || end->first > limit);
    // std::copy(timers_.begin(), end, back_inserter(expired));
    std::move(timers_.begin(), end, std::back_inserter(result));
    timers_.erase(timers_.begin(), end);    // erase from timers_
    for (const entry& cur_pair : result) {  // erase from activeTimers_
        int ret = activeTimers_.erase(cur_pair.second);
        (void)ret; assert(ret == 1);
    }
    
    assert(timers_.size() == activeTimers_.size());
    return result;  // rvo
}

void timer_queue::reset_repeat_timers(const std::vector<entry>& list, TimeStamp now) {
    for (const entry& item : list) {
        // 若当前定时器是"复用定时器"且没有被取消 则重新加入集合
        if (item.second->repeat() && cancelingTimers_.find(item.second) == cancelingTimers_.end()) {
            item.second->restart(now);
            // bool earliestChanged = insert(item.second);
            // There is no need to determine if the front has changed, because "reset_timers()" will be called
            insert(item.second);
        } else {
            delete item.second;
        }
    }
}

void timer_queue::reset_timer() const {
    TimeStamp nextExpiration;
    if (!timers_.empty()) {
        nextExpiration = timers_.begin().operator*().second->expiration();
        if (nextExpiration.valid()) {   // FIXME:此处仅判断了nextExpiration是否不为0，并没有判断nextExpiration是否大于TimeStamp::now()
            detail::reset_timerfd(timerfd_, nextExpiration);
        }
    }
}

void timer_queue::cancel(timer_id t_id) {
    loop_->run_in_eventLoop([this, t_id]() mutable {this->cancel_timer_inLoop(t_id.get_timer());});
}

void timer_queue::cancel_timer_inLoop(timer* t) {
    loop_->assert_loop_in_hold_thread();
    assert(timers_.size() == activeTimers_.size());
    activeTimerSet_t::iterator it = activeTimers_.find(t);
    if (it != activeTimers_.end()) {
        auto ret = timers_.erase(entry((*it)->expiration(), t));
        assert(ret == 1); (void)ret;
        activeTimers_.erase(it);
        delete t;
    } else if (handlingExpiredTimer_) {
        cancelingTimers_.insert(t);
    } else {
        LOG_FATAL << "A hidden bug !";
    }
    assert(timers_.size() == activeTimers_.size());
}