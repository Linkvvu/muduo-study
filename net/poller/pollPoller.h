// This is an internal header file, you should not include this.

#if !defined(MUDUO_NET_POLLER_POLLPOLLER_H)
#define MUDUO_NET_POLLER_POLLPOLLER_H

#include <muduo/net/poller.h>
#include <unordered_map>
#include <vector>

struct pollfd; // forward declaration for struct pollfd in header file sys/poll.h

namespace muduo {
namespace net {

///
/// IO Multiplexing with poll(2).
///
class poll_poller : public poller {
public:
    using poller::poller;   // continue to use constructor of the base class
    virtual ~poll_poller() override;

    virtual muduo::TimeStamp poll(const int timeoutMs, channelList_t* activeChannels) override;
    virtual void updateChannel(channel* channel) override;
    virtual void removeChannel(channel* channel) override;

private:
    void fill_activeChannels(int numReadyEvents, channelList_t* activeChannels) const;

    using pollFdList_t = std::vector<struct pollfd>;
    using channelMap_t = std::unordered_map<int, channel*>; // key是文件描述符，value是channel*

    pollFdList_t pollfds_;
    channelMap_t channels_;
};

} // namespace net 
} // namespace muduo 


#endif // MUDUO_NET_POLLER_POLLPOLLER_H
