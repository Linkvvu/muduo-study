#include <muduo/net/poller.h>

using namespace muduo;
using namespace muduo::net;

poller::poller(event_loop* loop) : ownerLoop_(loop) {}

poller::~poller() = default;