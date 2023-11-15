#include <muduo/poller/PollPoller.h>
#include <muduo/EventLoop.h>
#include <muduo/Callbacks.h>
#include <muduo/TimerQueue.h>
#include <muduo/Channel.h>
#include <muduo/Bridge.h>
#include <chrono>
#include <cassert>
#include <sys/poll.h>

namespace {
    thread_local muduo::EventLoop* tl_loop_inThisThread = {nullptr};
}

namespace muduo {
using namespace std::chrono;

const EventLoop::TimeoutDuration_t EventLoop::kPollTimeout = duration_cast<EventLoop::TimeoutDuration_t>(seconds(5));

EventLoop::EventLoop()
    : threadId_(::pthread_self())
    , looping_(false)
    , quit_(false)
    , eventHandling_(false)
    , poller_(std::make_unique<detail::PollPoller>(this))
    , activeChannels_()
    , timerQueue_(std::make_unique<TimerQueue>(this))
    , bridge_(std::make_unique<Bridge>(this))
    , callingPendingCbs_(false)
{
    LOG_DEBUG << "EventLoop is created in thread " << threadId_;
    if (tl_loop_inThisThread != nullptr) {
        LOG_FATAL << "Another EventLoop instance " << tl_loop_inThisThread
                << " exists in this thread";
    } else {
        tl_loop_inThisThread = this;
    }
}

EventLoop::~EventLoop() {
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
            << " destructs in thread " << ::pthread_self();
    tl_loop_inThisThread = nullptr;
}

EventLoop* EventLoop::GetCurrentThreadLoop() {
    return tl_loop_inThisThread;
}

void EventLoop::PrintActiveChannels() const {
    for (const Channel* channel : activeChannels_) {
        LOG_TRACE << "<" << channel->REventsToString() << "> ";
    }
}

void EventLoop::Loop() {
    AssertInLoopThread();   // must loop in itself thread
    assert(!looping_);
    assert(!eventHandling_);
    assert(!callingPendingCbs_);

    looping_ = true;
    quit_ = false;

    LOG_TRACE << "EventLoop " << this << " start to loop";

    while (!quit_) {
        activeChannels_.clear();    // Clear the list for polling new channels
        receiveTimePoint_ = poller_->Poll(kPollTimeout, &activeChannels_);
        HandleActiveChannels();
        if (muduo::GetLoglevel() <= Logger::LogLevel::TRACE) {
            PrintActiveChannels();
        }
        HandlePendingCallbacks();
    }
    looping_ = false;
}

void EventLoop::HandleActiveChannels() {
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

TimerId_t EventLoop::RunAfter(const Interval_t& delay, const TimeoutCb_t& cb) {
    using namespace std;
    auto timepoint = chrono::steady_clock::now() + delay;
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

void EventLoop::EnqueueEventLoop(const PendingEventCb_t& cb) {
    {
        std::lock_guard<std::mutex> guard(mtx_);
        pendingCbsQueue_.push_back(cb);
    }

    /**
     * 1. 如果不在IO线程中，因为IO线程此时可能阻塞在poll中，为确保任务即使被处理，故要调用WakeUp
     * 2. 如果在IO线程中并且此时线程正在处理pending Callbacks，由Loop内部实现决定此时还需调用WakeUp,防止IO线程阻塞
     * 3. 如果在IO线程中且此时线程正在处理activeChannels的callback，则无需调用WakeUp
    */
    if (!IsInLoopThread() || callingPendingCbs_) {
        bridge_->WakeUp();
    }
}

void EventLoop::RunInEventLoop(const PendingEventCb_t& cb) {
    if (IsInLoopThread()) {
        cb();
    } else {
        EnqueueEventLoop(cb);
    }
}

void EventLoop::HandlePendingCallbacks() {
    std::vector<PendingEventCb_t> tmp_queue;
    callingPendingCbs_.store(true);
    {   // 缩短临界区大小
        std::lock_guard<std::mutex> guard(mtx_);
        tmp_queue.swap(pendingCbsQueue_);    // Can effectively prevent deadlock
    }
    for (const auto& cb : tmp_queue) {
        cb.operator()();
    }
    callingPendingCbs_.store(false);
}

void EventLoop::Quit() {
    assert(quit_ == false);
    quit_.store(true);
    if (!IsInLoopThread()) {
        bridge_->WakeUp();
    }
}

}
