#if !defined(MUDUO_BRIDGE_H)
#define MUDUO_BRIDGE_H

#include <muduo/base/allocator/Allocatable.h>
#include <memory>

namespace muduo {

class EventLoop;    // forward declaration
class Channel;      // forward declaration


/**
 * Bridge for Multi-thread synchronization.
 * Be used as an event wait/notify mechanism
*/
#ifdef MUDUO_USE_MEMPOOL
class Bridge : public base::detail::Allocatable {
#else
class Bridge {
#endif
public:
    explicit Bridge(EventLoop* loop);
    ~Bridge() noexcept;
    void WakeUp() const;

private:
    void HandleWakeUpFdRead() const;

private:
    EventLoop* const owner_;
    std::unique_ptr<Channel> chan_; // wake-up fd channel
};

}

#endif // MUDUO_BRIDGE_H
