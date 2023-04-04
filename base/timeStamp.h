#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

#include "muduo/base/copyable.h"
#include <boost/operators.hpp>

namespace muduo {
    class TimeStamp : public muduo::copyable
                    , public boost::equality_comparable<TimeStamp>
                    , public boost::less_than_comparable<TimeStamp> {
public:
    ///
    /// Constucts an invalid Timestamp.
    ///
    TimeStamp() : microSecondsSinceEpoch_(0) {}

    ///
    /// Constucts a Timestamp at specific time
    ///
    TimeStamp(const int64_t microSecondsSinceEpochArg) : microSecondsSinceEpoch_(microSecondsSinceEpochArg) {}

    // default copy/assignment/dtor are Okay

    void swap(TimeStamp& that) {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    std::string toString() const;
    std::string toFormattedString(bool showMicroseconds = true) const;

    bool valid() const {
        return microSecondsSinceEpoch_ > 0;
    }


    // for internal usage.

    static const int kMicroSecondsPerSecond = 1000 * 1000; // microSecond与second的转换单位

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    time_t secondsSinceEpoch() const {
        return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }

    ///
    /// Get time of now.
    ///
    static TimeStamp now();
    static TimeStamp invaild() {
        return TimeStamp();
    }

    static TimeStamp fromUnixTime(time_t t) {
        return fromUnixTime(t, 0);
    }

    static TimeStamp fromUnixTime(time_t t, int microseconds) {
        return TimeStamp(static_cast<int64_t>(t) * kMicroSecondsPerSecond + microseconds);
    }

private:
    int64_t microSecondsSinceEpoch_;
};

inline bool operator<(TimeStamp lhs, TimeStamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(TimeStamp lhs, TimeStamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
/// @c double has 52-bit precision, enough for one-microsecond
/// resolution for next 100 years.
inline double timeDifference(TimeStamp high, TimeStamp low)
{
  int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
  return static_cast<double>(diff) / TimeStamp::kMicroSecondsPerSecond;
}

///
/// Add @c seconds to given timestamp.
///
/// @return target+seconds as Timestamp
///
inline TimeStamp addTime(TimeStamp target, double seconds)
{
  int64_t delta = static_cast<int64_t>(seconds * TimeStamp::kMicroSecondsPerSecond);
  return TimeStamp(target.microSecondsSinceEpoch() + delta);
}

}

#endif  // MUDUO_BASE_TIMESTAMP_H