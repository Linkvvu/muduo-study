#if !defined(MUDUO_BRIDGE_H)
#define MUDUO_BRIDGE_H

#include <muduo/base/allocator/mem_pool.h>
#include <memory>

namespace muduo {

class EventLoop;    // forward declaration
class Channel;      // forward declaration


/**
 * Bridge for Multi-thread synchronization.
 * Be used as an event wait/notify mechanism
*/
#ifdef MUDUO_USE_MEMPOOL
class Bridge final : public base::detail::ManagedByMempoolAble<Bridge> {
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
#ifdef MUDUO_USE_MEMPOOL
    std::unique_ptr<Channel, base::deleter_t<Channel>> chan_; // wake-up fd channel
#else
    std::unique_ptr<Channel> chan_; // wake-up fd channel
#endif
};

}

#endif // MUDUO_BRIDGE_H
