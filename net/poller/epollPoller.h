// This is an internal header file, you should not include this.

#include <muduo/net/poller.h>
#include <muduo/net/channel.h>
#include <unordered_map>
#include <vector>

struct epoll_event; // forward declaration

namespace muduo {
namespace net {

class epoll_poller : public poller{
public:
    epoll_poller(event_loop* const loop);
    virtual ~epoll_poller() override;
    virtual TimeStamp poll(int timeoutMs, channelList_t* activeChannels) override;
    virtual void updateChannel(channel* channel) override;
    virtual void removeChannel(channel* channel) override;
    
private:
    void fill_activeChannels(int numReadyEvents, channelList_t* activeChannels) const;
    void update(int operation, channel* const channel);

private:
    static const int kInitEventListSize = 16;
    
    // declaration custom type
    using epoll_eventList_t = std::vector<struct epoll_event>;
    using channelMap_t = std::unordered_map<int, channel*>; // key是文件描述符，value是channel*
    using fd_t = int;
    
    fd_t epollFD_;

    epoll_eventList_t events_;
    channelMap_t channels_;
};

} // namespace net 
} // namespace muduo 

