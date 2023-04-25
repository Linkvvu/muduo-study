#include <muduo/base/logging.h>
#include <muduo/net/poller/pollPoller.h>
#include <muduo/net/channel.h>
#include <sys/poll.h>
#include <cassert>

using namespace muduo;
using net::poll_poller;

poll_poller::~poll_poller() = default;

TimeStamp poll_poller::poll(const int timeoutMs, channelList_t* activeChannels) {
    int numReadyEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
    TimeStamp now(TimeStamp::now());
    if (numReadyEvents > 0) {
        LOG_TRACE << numReadyEvents << " events happended";
        fill_activeChannels(numReadyEvents, activeChannels);
    } else if (numReadyEvents == 0) {
        LOG_TRACE << "nothing happended";
    } else {
        LOG_SYSERR << "poll_poller::poll()";
    }
    return now; // RETURN timestamp when function poll() returns
}

void poll_poller::fill_activeChannels(int numReadyEvents, channelList_t* activeChannels) const {
    for (pollFdList_t::const_iterator c_it = pollfds_.begin();
            c_it != pollfds_.end() && numReadyEvents > 0; c_it++) {
        if (c_it->revents > 0) {
            numReadyEvents--;
            auto res = channels_.find(c_it->fd);    // 先判断该fd是否在关注的channel列表中
            assert(res != channels_.end());
            channel* cur_chan = res->second;    // 获取当前fd所对应的channel对象
            assert(cur_chan->fd() == res->first);
            cur_chan->set_revents(c_it->revents);
            // c_it->revents = 0;
            activeChannels->push_back(cur_chan);
        }
    }
}

void poll_poller::updateChannel(channel* channel) {
    poller::assert_loop_in_hold_thread();
    LOG_TRACE << "fd: " << channel->fd() << " events: " << channel->events();
    if (channel->index() < 0) { // index < 0,说明是一个新的通道,添加至pollfds_
        assert(channels_.find(channel->fd()) == channels_.end()); // 断言该fd没有对应的channel存在
        struct pollfd new_pollfd_obj;
        new_pollfd_obj.fd = channel->fd();
        new_pollfd_obj.events = static_cast<decltype(pollfd::events)>(channel->events());
        new_pollfd_obj.revents = 0;
        pollfds_.push_back(std::move(new_pollfd_obj));  // WARNING: new_pollfd_obj was moved!

        auto cur_struct_pollfd_idx = pollfds_.size() - 1;
        channel->set_index(cur_struct_pollfd_idx);
        channels_[channel->fd()] = channel;
    } else {    // 当前channel已存在，更新当前channel
        assert(channels_.find(channel->fd()) != channels_.end()); // 断言当前channel已存在
        assert(channels_[channel->fd()] == channel);        // 断言当前fd所关联的是原来的channel对象
        int idx = channel->index();
        assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
        struct pollfd& pfd = pollfds_[idx];
        pfd.events = static_cast<short>(channel->events());
        pfd.revents = 0;
	    // 将一个通道暂时更改为不关注事件，但不从Poller中移除该通道
        if (channel->isNoneEvent()) {
            // ignore this pollfd
            // 暂时忽略该文件描述符的事件
            // 这里pfd.fd 可以直接设置为-1
            pfd.fd = -channel->fd()-1;	// 这样子设置是为了优化removeChannel
        }
    }
}

void poll_poller::removeChannel(channel* channel) {
    poller::assert_loop_in_hold_thread();
    LOG_TRACE << "fd: " << channel->fd();
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    assert(channel->isNoneEvent()); // NOTE: 只有当前channel不关注任何事件才可以被从pollfds_中remove
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
    assert(pfd.fd == -channel->fd()-1 && static_cast<int>(pfd.events) == channel->events());

    // 将对应的channelMap移除
    auto ret = channels_.erase(channel->fd()); (void)ret;
    assert(ret == 1);
    
    // 将对应的struct pollfd移除
    if (static_cast<pollFdList_t::size_type>(channel->index()) == pollfds_.size() -1) {
        pollfds_.pop_back();
    } else {
	    // 这里移除的算法复杂度是O(1)，将待删除元素与最后一个元素交换再pop_back
        int channelAtEnd = pollfds_.back().fd;
        using std::swap;
        swap(pollfds_[idx], pollfds_.back());
        if (channelAtEnd < 0) { // 如果最后一个struct pollfd也是被忽略的
            channelAtEnd = -channelAtEnd - 1; // 先获取其真正的fd
        }
        channels_[channelAtEnd]->set_index(idx);
        pollfds_.pop_back();
    }
}