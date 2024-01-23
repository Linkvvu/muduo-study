#if !defined(MUDUO_POLLER_POLLPOLLER_H)
#define MUDUO_POLLER_POLLPOLLER_H

#include <muduo/Poller.h>
#include <unordered_map>

struct pollfd; // forward declaration for struct pollfd in header file sys/poll.h

namespace muduo {
namespace detail {

/**
 * IO Multiplexing with poll(2).
*/
class PollPoller : public Poller {
#ifdef MUDUO_USE_MEMPOOL
private:
    using PollFdList = std::vector<struct pollfd, muduo::base::allocator<struct pollfd>>;
    using ChannelMap = std::unordered_map<int, Channel*, std::hash<int>, std::equal_to<int>, base::allocator<std::pair<const int, Channel*>>>;
#else
private:
    using PollFdList = std::vector<struct pollfd>;
    using ChannelMap = std::unordered_map<int, Channel*>;
#endif

public:
    /// Constructor 
    PollPoller(EventLoop* loop);
    virtual ~PollPoller() noexcept override = default;
    
    virtual ReceiveTimePoint_t Poll(const TimeoutDuration_t& timeout, ChannelList* activeChannels);
    virtual void UpdateChannel(Channel* c) override;
    virtual void RemoveChannel(Channel* c) override;

private:
    void FillActiveChannels(int numReadyEvents, ChannelList* activeChannels) const;

private:
    PollFdList pollfds_;
    ChannelMap channels_;
};

} // namespace detail 
} // namespace muduo 

#endif // MUDUO_POLLER_POLLPOLLER_H
