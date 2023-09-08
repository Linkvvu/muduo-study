#include <EventLoop.h>
#include <chrono>
#include <sys/poll.h>

using namespace muduo;

namespace {
    thread_local EventLoop* tl_loop_inThisThread = nullptr;
}

EventLoop::EventLoop()
    : threadId_(std::this_thread::get_id())
    , looping_(false)
{
    std::clog << "thread " << threadId_ << " EventLoop is created" << std::endl;
    if (tl_loop_inThisThread != nullptr) {
        std::clog << "another EventLoop instance " << tl_loop_inThisThread
                << " exists in this thread(" << threadId_ << ")"
                << std::endl;
    } else {
        tl_loop_inThisThread = this;
    }
}

void EventLoop::Loop() {
    std::cout << "thread " << threadId_ << " starting loop" << std::endl;
    looping_ = true;
    ::poll(nullptr, 0, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(3)).count());
    looping_ = false;
}