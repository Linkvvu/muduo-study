#if !defined(MUDUO_EVENTLOOP_H)
#define MUDUO_EVENTLOOP_H

#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <muduo/base/Logging.h>
#include <muduo/TimerType.h>
#include <muduo/Callbacks.h>
#include <mutex>
#include <atomic>
#include <thread>
#include <forward_list>
#include <vector>
#include <memory>
#include <cassert>

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
    /// noncopyable & nonmoveable
    EventLoop(const EventLoop&) = delete;
    using PendingCallbacksQueue = std::vector<PendingEventCb_t>;
    
#ifdef MUDUO_USE_MEMPOOL
public:
    using ChannelList = std::forward_list<Channel*, base::allocator<Channel*>>;
#else
public:
    using ChannelList = std::vector<Channel*>;
#endif

public:
    using ReceiveTimePoint_t = std::chrono::system_clock::time_point;
    using TimeoutDuration_t = std::chrono::milliseconds;

    /// @brief Default constructor
    /// @return A EventLoop instance
    /// @note Only use by EventLoop::Create
    EventLoop();

    ~EventLoop();

    /// @brief Must be called in the same thread as creation of the object.
    void Loop();

    bool IsInLoopThread()
    { return threadId_ == ::pthread_self(); }

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
     
#ifdef MUDUO_USE_MEMPOOL
    base::MemoryPool* GetMemoryPool() {
        return memPool_.get();
    }
#endif

    static EventLoop* GetCurrentThreadLoop();
    
private:
    static const TimeoutDuration_t kPollTimeout;

private:
    /// @brief is not in holder-thread
    void Abort() {
        LOG_FATAL << "not in holder-thread, holder-thread-id=" << threadId_;
    }

    /**
     * debug helper
    */
    void PrintActiveChannels() const;
    void HandleActiveChannels();
    void HandlePendingCallbacks();

private:
#ifdef MUDUO_USE_MEMPOOL
    std::unique_ptr<base::MemoryPool> memPool_;  // Loop-level memory pool handle
#endif
    const pthread_t threadId_;
    bool looping_;
    std::atomic_bool quit_ { false };
    bool eventHandling_;
    std::unique_ptr<Poller> poller_;    // 组合
    std::unique_ptr<TimerQueue> timerQueue_;
    ReceiveTimePoint_t receiveTimePoint_;
    ChannelList activeChannels_;

    /* cross-threads wait/notify helper */
    std::unique_ptr<Bridge> bridge_;
    std::mutex mtx_;    // for sync EventLoop::pendingCbsQueue_
    PendingCallbacksQueue pendingCbsQueue_;
    std::atomic_bool callingPendingCbs_;
};

} // namespace muduo 

#endif // MUDUO_EVENTLOOP_H