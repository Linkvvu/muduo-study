#if !defined(MUDUO_TIMER_TYPE_H)
#define MUDUO_TIMER_TYPE_H

#include <chrono>
#include <functional>

namespace muduo {
namespace detail {

    using TimerId_t = int;
    using TimeoutCb_t = std::function<void()>;
    using TimePoint_t = std::chrono::steady_clock::time_point;
    using Interval_t = std::chrono::microseconds;

} // namespace detail 
} // namespace muduo 

#endif // MUDUO_TIMER_TYPE_H
