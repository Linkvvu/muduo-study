#include <muduo/net/channel.h>
#include <muduo/net/eventLoop.h>
#include <muduo/base/logging.h>
#include <sys/poll.h>
#include <cassert>

using namespace muduo;
using muduo::net::channel;

const int channel::kNoneEvent = 0;
const int channel::kReadEvent = POLLIN | POLLPRI;
const int channel::kWriteEvent = POLLOUT;

channel::channel(event_loop* loop, int fd)
    : owning_loop_(loop)
    , fd_(fd)
    , events_(kNoneEvent)
    , revents_(kNoneEvent)
    , index_(-1)
    , logHup_(true)
    , tie_()
    , tied_(false)
    , eventHandling_(false) {}


channel::~channel() {
    assert(eventHandling_ == false);
}

void channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

void channel::update() {
    owning_loop_->updateChannel(this);
}

// 调用这个函数之前确保调用disableAll
void channel::remove() {
    assert(isNoneEvent());
    owning_loop_->removeChannel(this);
}

void channel::handleEventWithGuard(TimeStamp recvTime) {
    eventHandling_ = true;
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) { // 对端挂起并且buf中没有数据可读了，则调用closeCallback
        if (logHup_) {
            LOG_WARN << "channel::hanle_event() POLLHUP";
        }
        if (closeCallback_) closeCallback_();
    }

    if (revents_ & POLLNVAL) {
        LOG_WARN << "channeL::hanle_event() POLLNVAL";
    }

    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) readCallback_(recvTime);
    }

    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }

    eventHandling_ = false;
}

void channel::handle_event(TimeStamp recvTime) {
    if (tied_) {
        std::shared_ptr<void> guard(tie_.lock());
        if (guard)
            handleEventWithGuard(recvTime);
    } else {
        handleEventWithGuard(recvTime);
    }
}

string channel::reventsToString() const {
    std::ostringstream oss;
    oss << fd_ << ": ";
    if (revents_ & POLLIN)
        oss << "IN ";
    if (revents_ & POLLPRI)
        oss << "PRI ";
    if (revents_ & POLLOUT)
        oss << "OUT ";
    if (revents_ & POLLHUP)
        oss << "HUP ";
    if (revents_ & POLLRDHUP)
        oss << "RDHUP ";
    if (revents_ & POLLERR)
        oss << "ERR ";
    if (revents_ & POLLNVAL)
        oss << "NVAL ";

    return oss.str();
}