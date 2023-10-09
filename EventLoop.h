#if !defined(MUDUO_EVENTLOOP_H)
#define MUDUO_EVENTLOOP_H

#include <TimerType.h>
#include <Callbacks.h>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>
#include <cassert>
#include <iostream>

/**
 * Mode of one EventLoop instance per thread.
 * For synchronize all threads,
 * All **Reactor-handlers** must handle IO events in their respective EventLoop-threads.
 */

namespace muduo {
    class TimerQueue;   // forward declaration
    class EventLoop;    // forward declaration
    class Channel;      // forward declaration
    class Poller;       // forward declaration
    class Bridge;       // forward declaration
}

namespace {
    extern thread_local muduo::EventLoop* tl_loop_inThisThread;
} // namespace 

namespace muduo {
    using namespace detail;
class EventLoop {
public:
    using ReceiveTimePoint_t = std::chrono::system_clock::time_point;
    using TimeoutDuration_t = std::chrono::milliseconds;
private:
    static const TimeoutDuration_t kPollTimeout;
public:
    EventLoop();
    ~EventLoop();
    /// noncopyable & nonmoveable
    EventLoop(const EventLoop&) = delete;

    /// @brief Must be called in the same thread as creation of the object.
    void Loop();

    bool IsInLoopThread()
    { return threadId_ == std::this_thread::get_id(); }

    void AssertInLoopThread()
    {
        if (!IsInLoopThread()) { Abort(); }
    }

    /// @note Safe to call from other threads
    void Quit();

    /// @note internal usage
    void UpdateChannel(Channel* c);
    /// @note internal usage
    void RemoveChannel(Channel* c);

    /**
     * Runs callback at 'when'
     * Safe to call from other threads
    */
    TimerId_t RunAt(const TimePoint_t& when, const TimeoutCb_t& cb);

    /**
     * Runs callback after delay which the given time
     * Safe to call from other threads
    */
    TimerId_t RunAfter(const Interval_t& delay, const TimeoutCb_t& cb);

    /**
     * Runs callback every 'interval' which the given time
     * Safe to call from other threads
    */
    TimerId_t RunEvery(const Interval_t& interval, const TimeoutCb_t& cb);

    /**
     * Safe to call from other threads.
    */
    void cancelTimer(const TimerId_t timerId);

    /**
     * Enqueueing cb in the loop thread
     * Runs after finish pooling
     * Safe to call from other threads
    */
    void EnqueueEventLoop(const PendingEventCb_t& cb);

    /**
     * Runs callback immediately in the loop thread
     * It wakes up the loop, and run the cb.
     * If in the same loop thread, cb is run within the function.
     * Safe to call from other threads.
    */
    void RunInEventLoop(const PendingEventCb_t& cb);
    
public:
    static EventLoop* GetCurrentThreadLoop();

private:
    /// @brief is not in holder-thread
    void Abort() {
        std::clog << "not in holder-thread, holder-thread = " 
                << threadId_ << ", but current thread = " 
                << std::this_thread::get_id()
                << std::endl;
        std::abort();
    }

    /**
     * debug helper
    */
    void PrintActiveChannels() const;
    void HandleActiveChannels();
    void HandlePendingCallbacks();

private:
    const std::thread::id threadId_;
    bool looping_;
    std::atomic_bool quit_ { false };
    bool eventHandling_;
    std::unique_ptr<Poller> poller_;    // 組合

    ReceiveTimePoint_t receiveTimePoint_;
    using ChannelList_t = std::vector<Channel*>;
    ChannelList_t activeChannels_;
    std::unique_ptr<TimerQueue> timerQueue_;

    /* cross-threads wait/notify helper */
    std::unique_ptr<Bridge> bridge_;
    std::mutex mtx_;    // for sync EventLoop::pendingCbsQueue_
    std::vector<PendingEventCb_t> pendingCbsQueue_;
    std::atomic_bool callingPendingCbs_;
};
    
} // namespace muduo 

#endif // MUDUO_EVENTLOOP_H