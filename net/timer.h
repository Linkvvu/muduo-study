// This is an internal header file, you should not include this.
#if !defined(MUDUO_NET_TIMER_H)
#define MUDUO_NET_TIMER_H
#include <muduo/base/timeStamp.h>
#include <muduo/base/logging.h>
#include <muduo/net/callBacks.h>
#include <boost/noncopyable.hpp>
#include <atomic>

namespace muduo {
namespace net {

class timer : private boost::noncopyable {
public:
    explicit timer(const TimeStamp expiration, const double interval, timerCallback_t func);
    ~timer() = default;
    bool repeat() const { return repeat_; }
    TimeStamp expiration() const  { return expiration_; }
    uint64_t sequence() const { return sequence_; }

    void run() const {
        if (callback_) 
            callback_();
        else 
            LOG_ERROR << "timer " << this << " have`t set callback!";
    }

    void restart(TimeStamp now);    // 若当前定时器不是一次性的，则设置下次的过期时间

public:
    static uint64_t createdCount() { return s_createdCount; }
    
private:
    static std::atomic<uint64_t> s_createdCount;
    timerCallback_t callback_;
    TimeStamp expiration_;  // 在当前时间戳过期
    const double interval_;
    const bool repeat_;
    const uint64_t sequence_;
};

} // namespace net 
} // namespace muduo 


#endif // MUDUO_NET_TIMER_H
