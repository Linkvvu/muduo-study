#include <poller/PollPoller.h>
#include <base/Logging.h>
#include <Channel.h>
#include <poll.h>
#include <iostream>
#include <cassert>
#include <cstring>

muduo::Poller::ReceiveTimePoint_t muduo::detail::PollPoller::Poll(const TimeoutDuration_t& timeout, ChannelList_t* activeChannels) {
    int numReady = ::poll(&pollfds_.front(), pollfds_.size(), timeout.count());
    // Get now-timestamp when poll(2) is awaked
    auto nowMicrosecond = std::chrono::system_clock::now();
    if (numReady > 0) {
        LOG_TRACE << numReady << " events happened.";
        FillActiveChannels(numReady, activeChannels);
    } else if (numReady == 0) {
        LOG_TRACE << numReady << " nothing happened";
    } else {
        if (errno != EINTR) {
            LOG_SYSERR << "PollPoller::Poll - " << muduo::strerror_thread_safe(errno);
        }
    }
    return nowMicrosecond;
}

void muduo::detail::PollPoller::UpdateChannel(Channel* c) {
    muduo::Poller::AssertInLoopThread();
    LOG_TRACE << "Update channel fd=" << c->FileDescriptor() << ", events=" << c->CurrentEvent();
    if (c->Index() < 0) {   // index < 0,说明是一个新的通道，添加当前channel
        assert(channels_.find(c->FileDescriptor()) == channels_.end()); // 断言该fd没有对应的channel存在
        struct pollfd new_pollfd_obj;
        new_pollfd_obj.fd = c->FileDescriptor();
        new_pollfd_obj.events = static_cast<decltype(pollfd::events)>(c->CurrentEvent());
        new_pollfd_obj.revents = 0;
        pollfds_.push_back(new_pollfd_obj);
        
        // set index for c
        std::size_t i = pollfds_.size() - 1;
        c->SetIndex(i);
        // put into the channel-list
        channels_[c->FileDescriptor()] = c;
    } else { // 当前channel已存在，更新当前channel
        assert(channels_.find(c->FileDescriptor()) != channels_.end());
        assert(channels_[c->FileDescriptor()] == c);
        assert(c->Index() >= 0 && c->Index() < pollfds_.size());
        struct pollfd* pfd = &pollfds_[c->Index()]; 
        pfd->events = static_cast<decltype(pollfd::events)>(c->CurrentEvent());
        pfd->revents = 0;

        // 将一个通道暂时更改为不关注事件，但不从Poller中移除该通道
        if (c->IsNoneEvent()) {
            // ignore this pollfd
            // 暂时忽略该文件描述符的事件
            // 这里pfd.fd 可以直接设置为-1
            pfd->fd = -(c->FileDescriptor()) - 1;	// 这样子设置是为了优化removeChannel
        }
    }
}

void muduo::detail::PollPoller::RemoveChannel(Channel* c) {
    Poller::AssertInLoopThread();
    LOG_TRACE << "Remove channel fd=" << c->FileDescriptor();
    assert(channels_.find(c->FileDescriptor()) != channels_.end());
    assert(channels_[c->FileDescriptor()] == c);
    assert(c->IsNoneEvent()); // NOTE:只有当前channel不关注任何事件才可以被从pollfds_中remove
    assert(c->Index() >= 0 && c->Index() < pollfds_.size());
    assert(pollfds_[c->Index()].fd == -(c->FileDescriptor())-1);

    // remove the channel from channel-list
    auto ret = channels_.erase(c->FileDescriptor());
    assert(ret == 1);   (void)ret;

    // remove the channel from struct-pollfd-list
    if (static_cast<PollfdList_t::size_type>(c->Index()) == pollfds_.size()-1) {
        pollfds_.pop_back();
    } else {
	    // 这里移除的算法复杂度是O(1)，将待删除元素与最后一个元素交换再pop_back
        int channel_at_end = pollfds_.back().fd;
        int idx = c->Index();
        using std::swap;
        swap(pollfds_[idx], pollfds_.back());

        if (channel_at_end < 0) { // 如果最后一个struct pollfd也是被忽略的
            channel_at_end = -(channel_at_end) - 1; // 先获取其真正的fd
        }
        channels_[channel_at_end]->SetIndex(idx);
        pollfds_.pop_back();    // remove
    }
}

void muduo::detail::PollPoller::FillActiveChannels(int numReadyEvents, ChannelList_t* activeChannels) const {
    for (PollfdList_t::const_iterator c_it = pollfds_.begin(); 
            c_it != pollfds_.end() && numReadyEvents > 0; c_it++)
    {
        if (c_it->revents > 0) {    // There are interesting events coming
            numReadyEvents -= 1;
            auto target_pair = channels_.find(c_it->fd);
            assert(target_pair != channels_.end());
            Channel* current_target_channel = target_pair->second;
            assert(target_pair->first == current_target_channel->FileDescriptor());
            current_target_channel->Set_REvent(c_it->revents);
            activeChannels->push_back(current_target_channel);
        }
    }
}
