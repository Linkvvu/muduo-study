#include <muduo/net/eventLoop.h>
#include <muduo/net/channel.h>
#include <muduo/net/poller.h>
#include <muduo/base/logging.h>
#include <poll.h>
#include <cassert>

using muduo::net::event_loop;


namespace { // Global scope
// 当前线程EventLoop对象指针
// 线程局部存储
__thread event_loop* t_loopInThisThread = nullptr;
const int KPollTime_Ms = 10 * 1000;

} // Global scope


namespace muduo {
namespace net {

event_loop* event_loop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

event_loop::event_loop()
    : looping_(false)
    , quit_(false)
    , eventHandling_(false)
    , holdThreadId_(currentThread::tid())
    , pollReturn_timeStamp_()
    , poller_(poller::newDefaultPoller(this))
    , activeChannles_()
    , cur_activeChannel_(nullptr) {
        LOG_TRACE << "EventLoop created " << this << "in thread" << holdThreadId_;
        if (t_loopInThisThread != nullptr) {
            LOG_FATAL << "Anothrer event_loop object: " << t_loopInThisThread
                      << " exists in this thread: " << holdThreadId_; 
        } else {
            t_loopInThisThread = this;
        }
    }

event_loop::~event_loop() {
    t_loopInThisThread = nullptr;
}


// 事件循环，该函数不能跨线程调用
// 只能在创建该对象的线程中调用
void event_loop::loop() {
    assert_loop_in_hold_thread();
    assert(!looping_);
    assert(!eventHandling_);
    looping_ = true;
    LOG_TRACE << "event-loop: " << this << " start looping";
    while (!quit_) {
        activeChannles_.clear();
        pollReturn_timeStamp_ = poller_->poll(KPollTime_Ms, &activeChannles_);
        
        if (logger::logLevel() <= logger::LogLevel::TRACE) {
            printActiveChannels();
        }
        
        eventHandling_ = true;
        for (channelList_t::iterator it = activeChannles_.begin(); it != activeChannles_.end(); it++) {
            cur_activeChannel_ = *it;
            cur_activeChannel_->handle_event(pollReturn_timeStamp_);
        }
        cur_activeChannel_ = nullptr;
        eventHandling_ = false;
    }
    LOG_TRACE << "event-loop: " << this << " stop looping";
    looping_ = false;
}

void event_loop::printActiveChannels() const {
    for (channelList_t::const_iterator it = activeChannles_.begin(); it != activeChannles_.end(); it++) {
        (*it)->reventsToString();
    }
}

void event_loop::abort_for_not_in_holdThread() {
    LOG_FATAL << "event_loop::abort_for_not_in_holdThread - " << this
              << " was created in thread: " << holdThreadId_
              << ", current thread id: " << currentThread::tid();
}

void event_loop::updateChannel(channel* channel) {
    poller_->updateChannel(channel);
}

void event_loop::removeChannel(channel* channel) {
    poller_->removeChannel(channel);
}

} // namespace net 
} // namespace muduo
