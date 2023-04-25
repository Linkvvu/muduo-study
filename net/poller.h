// This is an internal header file, you should not include this.
#if !defined(MUDUO_NET_POLLER_H)
#define MUDUO_NET_POLLER_H

#include <muduo/base/timeStamp.h>
#include <muduo/net/eventLoop.h>
#include <boost/noncopyable.hpp>
#include <vector>

namespace muduo {
namespace net {

class channel;

///
/// Abstracted base class for IO Multiplexing
///
/// This class doesn't own the Channel objects.
class poller : boost::noncopyable {
public:
    using channelList_t = std::vector<channel*>;

    static poller* newDefaultPoller(event_loop* loop);

    poller(event_loop* loop);
    virtual ~poller(); // Abstracted base class must have a virtual destructor

    /// Polls the I/O events.
    /// Must be called in the loop thread.
    virtual muduo::TimeStamp poll(const int timeoutMs, channelList_t* activeChannels) = 0;

    /// Changes or add the interested I/O events.
    /// Must be called in the loop thread.
    virtual void updateChannel(channel* channel) = 0;

    /// Remove the channel, when it destructs.
    /// Must be called in the loop thread.
    virtual void removeChannel(channel* channel) = 0;

    void assert_loop_in_hold_thread() {
        ownerLoop_->assert_loop_in_hold_thread();
    }

private:
    event_loop* const ownerLoop_;	// Poller所属EventLoop
};

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_POLLER_H
