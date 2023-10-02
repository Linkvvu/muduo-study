#if !defined(MUDUO_POLLER_POLLPOLLER_H)
#define MUDUO_POLLER_POLLPOLLER_H

#include <Poller.h>
#include <unordered_map>

struct pollfd; // forward declaration for struct pollfd in header file sys/poll.h

namespace muduo {
namespace detail {

/**
 * IO Multiplexing with poll(2).
*/
class PollPoller : public Poller {
    using PollfdList_t = std::vector<struct pollfd>;
    using ChannelMap_t = std::unordered_map<int, Channel*>;
public:
    using Poller::Poller;   // continue to use constructor of the base class
    virtual ~PollPoller() noexcept override = default;
    
    virtual ReceiveTimePoint_t Poll(const TimeoutDuration_t& timeout, ChannelList_t* activeChannels);
    virtual void UpdateChannel(Channel* c) override;
    virtual void RemoveChannel(Channel* c) override;

private:
    void FillActiveChannels(int numReadyEvents, ChannelList_t* activeChannels) const;

private:
    PollfdList_t pollfds_;
    ChannelMap_t channels_;
};

} // namespace detail 
} // namespace muduo 

#endif // MUDUO_POLLER_POLLPOLLER_H
