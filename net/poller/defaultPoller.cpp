#include <muduo/net/poller/pollPoller.h>
#include <sys/poll.h>

using namespace muduo::net;

poller* poller::newDefaultPoller(event_loop* loop) {
    return new poll_poller(loop);
}