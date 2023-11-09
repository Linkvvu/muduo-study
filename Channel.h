#if !defined(MUDUO_CHANNEL_H)
#define MUDUO_CHANNEL_H

#include <EventLoop.h>
#include <functional>
#include <memory>
#include <chrono>
#include <cassert>

namespace muduo {

/**
 * This class doesn't own the file descriptor.
 * The file descriptor could be a socket \ event fd \ timer fd \ signal fd
 * Channel is responsible to register and handle IO events
*/
class Channel {
    Channel(const Channel&) = delete;   // non-copyable & non-moveable
    
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

public:
    Channel(EventLoop* owner, int fd);
    using EventCallback_t = std::function<void()>;
    using ReceiveTimePoint_t = EventLoop::ReceiveTimePoint_t;
    using RDCallback_t = std::function<void(const ReceiveTimePoint_t&)>;

    ~Channel() { 
        assert(!eventHandling_);
        /// TODO: assert(!addedToLoop_);
    };

    /**
     * 将当前Channel绑定給給由shared_ptr管理的所有者对象
     * 防止所有者对象在Poller::handleEvent中被销毁
    */
    void Tie(const std::shared_ptr<void>& obj) {
        tie_ = obj;
        tied_ = true;
    }

    void SetReadCallback(const RDCallback_t& cb) { readCb_ = cb; }
    void SetWriteCallback(const EventCallback_t& cb) { writeCb_ = cb; }
    void SetCloseCallback(const EventCallback_t& cb) { closeCb_ = cb; }
    void SetErrorCallback(const EventCallback_t& cb) { errorCb_ = cb; }

    void EnableReading() { events_ |= kReadEvent; Update(); }
    void enableWriting() { events_ |= kWriteEvent; Update(); }
    void disableWriting() { events_ &= ~kWriteEvent; Update(); }
    void disableAllEvents() { events_ = kNoneEvent; Update(); }

    bool IsNoneEvent() const { return events_ == kNoneEvent; }
    bool IsWriting() const { return events_ & kWriteEvent; }
    void NotLogHup() { logHup_ = false; }

    /**
     * handle incoming events
    */
    void HandleEvents(ReceiveTimePoint_t receiveTime);

    /**
     * return the loop instance whos own the current channel
    */
    EventLoop* Owner() { return owner_; }
    const EventLoop* Owner() const { return owner_; }

    int FileDescriptor() const { return fd_; }
    int CurrentEvent() const { return events_; }
    int Index() const { return index_; }


    /**
     * used by pollers
    */
    void Set_REvent(const int revt) { revents_ = revt; }
    void SetIndex(const std::size_t i) { index_ = i; }


    void Remove();

    /**
     * for debug
    */
    std::string REventsToString() const {
        // std::cout << "called Channel::REventsToString" << std::endl;
        return "";
    }
    
private:
    void Update();
    void HandleEventsWithGuard(ReceiveTimePoint_t receiveTime);

private:
    EventLoop* owner_;  // 聚合关系 (1 channel only belong to 1 loop instance)
    const int fd_;
    int events_;        // interested I/O events
    bool logHup_;       // for POLLHUP
    bool eventHandling_; 
    int revents_;		// poll/epoll返回的事件
    int index_;

    bool tied_;
    std::weak_ptr<void> tie_;
    
    RDCallback_t  readCb_;
    EventCallback_t writeCb_;
    EventCallback_t closeCb_;
    EventCallback_t errorCb_;
};

} // namespace muduo 


#endif // MUDUO_CHANNEL_H
