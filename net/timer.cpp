#include <muduo/net/timer.h>

using namespace muduo;
using namespace net;

// The numbering starts at 1
std::atomic<uint64_t> timer::s_createdCount(1);

timer::timer(const TimeStamp expiration, const double interval, timerCallback_t func)
    : callback_(func)
    , expiration_(expiration)
    , interval_(interval)
    , repeat_(interval > 0.0)
    , sequence_(s_createdCount++) {}

void timer::restart(TimeStamp now) {
    if (repeat_) {
        expiration_ = addTime(now, interval_);
    } else {
        expiration_ = TimeStamp::invalid();
    }
}