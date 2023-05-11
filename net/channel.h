#if !defined(MUDUO_NET_CHANNEL_H)
#define MUDUO_NET_CHANNEL_H

#include <muduo/base/timeStamp.h>
#include <muduo/base/Types.h>
#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>

namespace muduo {
namespace net {

class event_loop;   // forward declaration

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a si gnalfd
/// 负责注册与响应IO事件,但不拥有file descriptor
class channel : private boost::noncopyable {
public:
    using eventCallback = std::function<void()>;
    using read_eventCallback = std::function<void(TimeStamp)>;

    channel(event_loop* loop, int fd);
    ~channel();

    /// Tie this channel to the owner object managed by shared_ptr,
    /// prevent the owner object being destroyed in handleEvent.
    void tie(const std::shared_ptr<void>&);

    void setReadCallback(const read_eventCallback& cb) { readCallback_ = cb; }
    void setWriteCallback(const eventCallback& cb) { writeCallback_ = cb; }
    void setCloseCallback(const eventCallback& cb) { closeCallback_ = cb; }
    void setErrorCallback(const eventCallback& cb) { errorCallback_ = cb; }

    void enableReading() { events_ |= kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAllEvents() { events_ = kNoneEvent; update(); }

    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    void doNotLogHup() { logHup_ = false; }

    int fd() const { return fd_; }
    event_loop* ownerLoop() { return owning_loop_; }
    int events() const { return events_; }
    int revents() const { return revents_; }

    void set_revents(int revt) { revents_ = revt; } // used by pollers

    // for poll_poller
    int index() const { return index_; }
    void set_index(int idx) { index_ = idx; }
    
    void handle_event(TimeStamp recvTime);
    void remove();

    // for debug
    string eventsToString(int events) const;

private:
    void update();
    void handleEventWithGuard(TimeStamp receiveTime);

private:
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

private:
    event_loop* const owning_loop_;   // 聚合关系(双向关联)，注册其所属event-loop
    const int fd_;
    int        events_;		    // 关注的事件
    int        revents_;		// poll/epoll返回的事件
    int        index_;		    // used by Poller.表示在poll的事件数组中的序号
    bool       logHup_;		    // for POLLHUP

    std::weak_ptr<void> tie_;
    bool tied_;
    bool eventHandling_;        // 是否正在处理事件
    read_eventCallback readCallback_;
    eventCallback writeCallback_;
    eventCallback closeCallback_;
    eventCallback errorCallback_;
};

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_CHANNEL_H
