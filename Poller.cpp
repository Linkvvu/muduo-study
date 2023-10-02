#include <Poller.h>
#include <EventLoop.h>

using namespace muduo;

Poller::Poller(EventLoop* loop)
    : loop_(loop) {}

void Poller::AssertInLoopThread() {
    loop_->AssertInLoopThread();
}
