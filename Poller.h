#if !defined(MUDUO_POLLER_H)
#define MUDUO_POLLER_H

#include <muduo/base/allocator/Allocatable.h>
#include <muduo/EventLoop.h>
#include <vector>
#include <chrono>


namespace muduo {

class Channel;      // forward declaration
class EventLoop;    // forward declaration

/**
 * abstract base class for IO-multiplexing
 * this class doesn`t own the Channel objects
 * 使用IO-multiplexing，将轮询到的结果(coming IO-event)赋值给fd对应的Channel，将Channels返回给Loop
 * Loop根据channels的coming IO-event挨个调用(ensure thread-safely)各自设置的handle functions, 以响应IO-event
*/

#ifdef MUDUO_USE_MEMPOOL
class Poller : public base::detail::Allocatable {
#else
class Poller {
#endif
protected:
    using ChannelList = EventLoop::ChannelList;
    Poller(const Poller&) = delete; // non-copyable
    Poller(Poller&&) = delete;      // non-moveable

public:
    using ReceiveTimePoint_t = EventLoop::ReceiveTimePoint_t;
    using TimeoutDuration_t = std::chrono::milliseconds;

    Poller(EventLoop* loop);
    
    virtual ~Poller() noexcept = default;
    
    /**
     * Polls file descriptor for I/O events.
     * Must be called in the loop thread.
    */
    virtual ReceiveTimePoint_t Poll(const TimeoutDuration_t& timeout, ChannelList* activeChannels) = 0;

    /**
     * call by EventLoop instance
     * Changes or add the interested I/O events.
     * Must be called in the loop thread.
    */
    virtual void UpdateChannel(Channel* c) = 0;

    /**
     * call by EventLoop instance
     * Remove the channel, when it destructs.
     * Must be called in the loop thread.
    */
    virtual void RemoveChannel(Channel* c) = 0;

    void AssertInLoopThread();

private:
    EventLoop* loop_;	// Poller instance belong to an EventLoop obj
};

}

#endif // MUDUO_POLLER_H
