#include <muduo/net/poller/epollPoller.h>
#include <muduo/base/logging.h>
#include <muduo/net/channel.h>
#include <sys/epoll.h>
#include <cassert>


using namespace muduo;
using net::epoll_poller;

namespace { // Golab

const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

}

epoll_poller::epoll_poller(event_loop* const loop)
    : poller(loop)
    , epollFD_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize, ::epoll_event()) {
        if (epollFD_ == -1) {
            LOG_SYSFATAL << "epoll_poller::epoll_poller <constructor>";
        }
}

epoll_poller::~epoll_poller() {
    if (::close(epollFD_) != 0) {
        LOG_SYSERR << "fail to close epollFD";
    }
}

TimeStamp epoll_poller::poll(int timeoutMs, channelList_t* activeChannels) {
    int eventNum = ::epoll_wait(epollFD_, &events_.front(), static_cast<int>(events_.size()), timeoutMs);
    TimeStamp now(TimeStamp::now());
    if (eventNum > 0) {
        LOG_TRACE << eventNum << " events happended";
        fill_activeChannels(eventNum, activeChannels);
        if (static_cast<std::size_t>(eventNum) == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (eventNum == 0) {
        LOG_TRACE << " nothing happended";
    } else {
        LOG_SYSERR << "epoll_poller::poll()";
    }
    return now;
}

void epoll_poller::fill_activeChannels(int numReadyEvents, channelList_t* activeChannels) const {
    assert(static_cast<std::size_t>(numReadyEvents) <= events_.size());
    for (int i = 0; i < numReadyEvents; i++) {
        channel* const cur = static_cast<channel*>(events_[i].data.ptr);
#if !defined(NDEBUG)
        const fd_t fd = cur->fd();
        channelMap_t::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == cur);
#endif // NDEBUG
        cur->set_revents(events_[i].events);
        activeChannels->push_back(cur);
    }
}

void epoll_poller::updateChannel(channel* channel)  {
    poller::assert_loop_in_hold_thread();
    LOG_TRACE << "fd: " << channel->fd() << " events: " << channel->eventsToString(channel->events());
    const int state = channel->index();
    if (state == kNew || state == kDeleted) {   // a new one, add with EPOLL_CTL_ADD
        const fd_t fd = channel->fd();
        if (state == kNew) {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else if (state == kDeleted) {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);
        }
        channel->set_index(kAdded); // 将状态设置为已添加
        update(EPOLL_CTL_ADD, channel);
    } else {    // update existing one with EPOLL_CTL_MOD/DEL
        const fd_t fd = channel->fd(); (void)fd;;
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(channel->index() == kAdded);
        // if current channel attention to nothing, set state to Delete but Not removed from the channels_
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else { // current channel attention to some another things
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void epoll_poller::removeChannel(channel* channel) {
    poller::assert_loop_in_hold_thread();
    fd_t fd = channel->fd();
    assert(channels_.find(fd) != channels_.end() && channels_[fd] == channel);
    assert(channel->index() == kDeleted);    
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    auto ret = channels_.erase(fd); // remove current channel from channels_
    (void)ret; assert(ret == 1);
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);   // because current channel has been remove from map, so It`s a new channel
    LOG_TRACE << "fd: " << fd;
}

void epoll_poller::update(int operation, channel* const channel) {
    struct epoll_event event {0};
    event.data.ptr = channel;
    event.events = channel->events();
    fd_t fd = channel->fd();
    if (::epoll_ctl(epollFD_, operation, fd, &event) != 0) {
        if (operation == EPOLL_CTL_DEL) {
            LOG_SYSERR << "epoll_ctl op: " << operation << " fd = " << fd;
        } else {    // EPOLL_CTL_ADD/EPOLL_CTL_MOD
            LOG_SYSFATAL << "epoll_ctl op: " << operation << " fd = " << fd;
        }
    }
}
