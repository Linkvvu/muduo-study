#include <poller/PollPoller.h>
#include <EventLoop.h>
#include <TimerQueue.h>
#include <Channel.h>
#include <chrono>
#include <cassert>
#include <sys/poll.h>

using namespace muduo;
using namespace detail;

namespace {
    thread_local EventLoop* tl_loop_inThisThread = nullptr;
}

using namespace std::chrono;

const EventLoop::TimeoutDuration_t EventLoop::kPollTimeout = duration_cast<EventLoop::TimeoutDuration_t>(seconds(5));

EventLoop::EventLoop()
    : threadId_(std::this_thread::get_id())
    , looping_(false)
    , quit_(false)
    , eventHandling_(false)
    , poller_(std::make_unique<detail::PollPoller>(this))
    , curChannel_(nullptr)
    , activeChannels_()
    , timerQueue_(std::make_unique<TimerQueue>(this))
{
    std::clog << "thread " << threadId_ << " EventLoop is created" << std::endl;
    if (tl_loop_inThisThread != nullptr) {
        std::cerr << "another EventLoop instance " << tl_loop_inThisThread
                << " exists in this thread(" << threadId_ << ")"
                << std::endl;
        abort();
    } else {
        tl_loop_inThisThread = this;
    }
}

EventLoop::~EventLoop() {
    tl_loop_inThisThread = nullptr;
}

void EventLoop::PrintActiveChannels() const {
    
}

void EventLoop::Loop() {
    AssertInLoopThread();   // must loop in itself thread
    assert(!looping_);
    assert(!eventHandling_);

    looping_ = true;
    std::cout << "thread " << threadId_ << " starting loop" << std::endl;

    while (!quit_) {
        activeChannels_.clear();    // Clear the list for polling new channels
        receiveTimePoint_ = poller_->Poll(kPollTimeout, &activeChannels_);
        HandleActiveChannels();
        // if (logger::logLevel() <= logger::LogLevel::TRACE) {
        //     printActiveChannels();
        // }
    }
    looping_ = false;
}

void EventLoop::HandleActiveChannels() const {
    assert(!eventHandling_);
    eventHandling_ = true;
    for (auto c : activeChannels_) {
        c->HandleEvents(receiveTimePoint_);
    }
    eventHandling_ = false;
}

void EventLoop::UpdateChannel(Channel* c) {
    poller_->UpdateChannel(c);
}

void EventLoop::RemoveChannel(Channel* c) {
    poller_->RemoveChannel(c);
}

TimerId_t EventLoop::RunAt(const TimePoint_t& when, const TimeoutCb_t& cb) {
    return timerQueue_->AddTimer(when, TimeoutDuration_t::zero(), cb);
}

TimerId_t EventLoop::RunAfter(const Interval_t& delay_ms, const TimeoutCb_t& cb) {
    using namespace std;
    auto timepoint = chrono::steady_clock::now() + delay_ms;
    return RunAt(timepoint, cb);
}

TimerId_t EventLoop::RunEvery(const Interval_t& interval, const TimeoutCb_t& cb) {
    using namespace std;
    auto timepoint = chrono::steady_clock::now() + interval;
    return timerQueue_->AddTimer(timepoint, interval, cb);
}

void EventLoop::cancelTimer(const TimerId_t timerId) {
    timerQueue_->CancelTimer(timerId);
}
