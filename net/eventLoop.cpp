#include <muduo/net/eventLoop.h>
#include <muduo/base/logging.h>
#include <poll.h>
#include <cassert>

using muduo::net::event_loop;


namespace { // Global scope
// 当前线程EventLoop对象指针
// 线程局部存储
__thread event_loop* t_loopInThisThread = nullptr;

} // Global scope


namespace muduo {
namespace net {

event_loop* event_loop::getEventLoopOfCurrentThread() {
    return t_loopInThisThread;
}

event_loop::event_loop() :  looping_(false), holdThreadId_(currentThread::tid()) {
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

void event_loop::loop() {
    assert(!looping_);
    assert_loop_in_hold_thread();
    looping_ = true;
    LOG_TRACE << "event_loop: " << this << " start looping";
    ::poll(nullptr, 0, 5*1000); // temp(wait 5 seconds)
    LOG_TRACE << "event_loop: " << this << " stop looping";
    looping_ = false;
}

void event_loop::abort_for_not_in_holdThread() {
    LOG_FATAL << "event_loop::abort_for_not_in_holdThread - " << this
              << " was created in thread: " << holdThreadId_
              << ", current thread id: " << currentThread::tid();
}

} // namespace net 
} // namespace muduo
