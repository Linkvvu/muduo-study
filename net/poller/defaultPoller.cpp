#include <muduo/net/poller/pollPoller.h>
#include <muduo/net/poller/epollPoller.h>
#include <sys/poll.h>
#include <stdlib.h>

using namespace muduo::net;

poller* poller::newDefaultPoller(event_loop* loop) {
    if (::getenv("MUDUO_USE_POLL")) {
        return new poll_poller(loop);
    } else {
        return new epoll_poller(loop);
    }
}