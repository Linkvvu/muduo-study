#include <muduo/net/eventLoop.h>
#include <muduo/net/channel.h>
#include <muduo/net/poller.h>
#include <muduo/base/logging.h>
#include <muduo/net/timerId.h>
#include <muduo/net/timerQueue.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <cassert>

using muduo::net::event_loop;


namespace { // Global scope
// 当前线程EventLoop对象指针
// 线程局部存储
__thread event_loop* t_loopInThisThread = nullptr;
const int KPollTime_Ms = 100 * 1000;

} // Global scope


namespace muduo {
namespace net {

namespace detail {

int create_eventFd() {
    int fd = ::eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
    if (fd == -1) {
        LOG_SYSFATAL << "fail to create event fd for wake up";
    }
    return fd;
}

} // namespace detail 

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
    , cur_activeChannel_(nullptr)
    , timers_(std::make_unique<timer_queue>(this))
    , wakeupFdChannel_(std::make_unique<channel>(this, detail::create_eventFd(), "wake-up"))
    , handlingPendingFunctors_(false)
    , mutex_()
    , pendingFunctorQueue_()
    {
        LOG_TRACE << "EventLoop created " << this << " in thread" << holdThreadId_;
        if (t_loopInThisThread != nullptr) {
            LOG_FATAL << "Anothrer event_loop object: " << t_loopInThisThread
                      << " exists in this thread: " << holdThreadId_; 
        } else {
            t_loopInThisThread = this;
        }

        wakeupFdChannel_->setReadCallback(std::bind(&event_loop::handle_wakeupFd_read, this));
        wakeupFdChannel_->enableReading();
    }

event_loop::~event_loop() {
    LOG_DEBUG << "event_loop " << this << " of thread " << holdThreadId_
        << " destructs in thread " << currentThread::tid();
    wakeupFdChannel_->disableAllEvents();
    wakeupFdChannel_->remove();
    ::close(wakeupFdChannel_->fd());
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
        handle_pendingFunctors();
    }
    LOG_TRACE << "event-loop: " << this << " stop looping";
    looping_ = false;
}

void event_loop::printActiveChannels() const {
    for (channelList_t::const_iterator it = activeChannles_.begin(); it != activeChannles_.end(); it++) {
        LOG_TRACE << "{ " << (*it)->eventsToString((*it)->revents()) << "}";
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

timer_id event_loop::run_at(const TimeStamp& time, const timerCallback_t& func) {
    return timers_->add_timer(time, 0.0, func);
}

timer_id event_loop::run_after(double delay, const timerCallback_t& func) {
    TimeStamp target_point = addTime(TimeStamp::now(), delay);
    return run_at(target_point, func);
}

timer_id event_loop::run_every(double interval, const timerCallback_t& func) {
    TimeStamp target_point(addTime(TimeStamp::now(), interval));
    return timers_->add_timer(target_point, interval, func);
}

void event_loop::cancel(timer_id t) {
    timers_->cancel(t);
}


void event_loop::wakeup() {
    uint64_t buffer = 1;
    auto n = ::write(wakeupFdChannel_->fd(), &buffer, sizeof buffer);
    if (n != sizeof buffer) {
        LOG_ERROR << "event_loop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void event_loop::handle_wakeupFd_read() {
    uint64_t buffer = 1;
    ssize_t n = ::read(wakeupFdChannel_->fd(), &buffer, sizeof buffer);
    LOG_TRACE << "fd" << wakeupFdChannel_->fd() << "(" << wakeupFdChannel_->friendly_name() << "): " << buffer;
    if (n != sizeof buffer) {
        LOG_ERROR << "event_loop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void event_loop::enqueue_eventLoop(const pendingCallback_t& func) {
    {   // thread safe enqueue
        muduo::mutexLock_guard locker(mutex_);
        pendingFunctorQueue_.push_back(func);
    }

    // 1. 如果不在IO线程中，因为IO线程此时可能阻塞在poll中，为确保任务即使被处理，故要调用wakeup
    // 2. 如果在IO线程中并且此时线程正在处理待决functors，由poll内部实现决定此时还需调用wakeup,防止IO线程阻塞
    // 3. 如果在IO线程中且此时线程正在处理activeChannels的callback，则无需调用wakeup
    if (!is_in_eventLoop_thread() || handlingPendingFunctors_) {
        wakeup();
    }
}

void event_loop::handle_pendingFunctors() {
    std::vector<pendingCallback_t> tmpQueue;
    handlingPendingFunctors_ = true;
    {   // 缩短临界区大小
        muduo::mutexLock_guard locker(mutex_);
        tmpQueue.swap(pendingFunctorQueue_);    // Can effectively prevent deadlock
    }
    for (const pendingCallback_t& functor : tmpQueue) {
        functor();
    }
    handlingPendingFunctors_ = false;
}

void event_loop::run_in_eventLoop_thread(const pendingCallback_t& func) {
    if (is_in_eventLoop_thread()) {
        func();
    } else {
        enqueue_eventLoop(func);
    }
}

} // namespace net 
} // namespace muduo
