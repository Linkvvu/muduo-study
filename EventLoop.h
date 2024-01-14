#if !defined(MUDUO_EVENTLOOP_H)
#define MUDUO_EVENTLOOP_H

#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <muduo/base/Logging.h>
#include <muduo/TimerType.h>
#include <muduo/Callbacks.h>
#include <mutex>
#include <atomic>
#include <thread>
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

#ifdef MUDUO_USE_MEMPOOL
class EventLoop final : public base::detail::ManagedByMempoolAble<EventLoop> {
#else
class EventLoop {
#endif
    /// noncopyable & nonmoveable
    EventLoop(const EventLoop&) = delete;

#ifdef MUDUO_USE_MEMPOOL
    public:
    /// @brief Factory pattern
    /// @return A EventLoop instance within muduo::base::memory_pool 
    static std::unique_ptr<EventLoop, base::deleter_t<EventLoop>> Create();

    private:
    /// Construct with memory pool, prevent create on stack 
    /// @note Only use by EventLoop::Create
    EventLoop(const std::shared_ptr<base::MemoryPool>& pool);
#else
    public:
    /// @return A EventLoop instance
    static std::unique_ptr<EventLoop> Create();

    private:
    /// @brief Default constructor
    /// @return A EventLoop instance, don't use muduo::base::memory_pool 
    /// @note Only use by EventLoop::Create
    EventLoop();
#endif
    
public:
    using ReceiveTimePoint_t = std::chrono::system_clock::time_point;
    using TimeoutDuration_t = std::chrono::milliseconds;
#ifdef MUDUO_USE_MEMPOOL
    using ChannelList = std::vector<Channel*, base::allocator<Channel*>>;
    private:
    using PendingCallbacksQueue = std::vector<PendingEventCb_t, base::allocator<PendingEventCb_t>>; 
#else
    using ChannelList = std::vector<Channel*>;
    private:
    using PendingCallbacksQueue = std::vector<PendingEventCb_t>;
#endif
    static const TimeoutDuration_t kPollTimeout;

public:
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
    const std::shared_ptr<base::MemoryPool>& GetMemoryPool() const {
        return memPool_;
    }
#endif

public:
    static EventLoop* GetCurrentThreadLoop();

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
    const pthread_t threadId_;
    bool looping_;
    std::atomic_bool quit_ { false };
    bool eventHandling_;
#ifdef MUDUO_USE_MEMPOOL
    const std::shared_ptr<base::MemoryPool> memPool_;  // Loop-level memory pool handle
    std::unique_ptr<Poller, base::deleter_t<Poller>> poller_;    // 组合
    std::unique_ptr<TimerQueue, base::deleter_t<TimerQueue>> timerQueue_;
#else
    std::unique_ptr<Poller> poller_;    // 组合
    std::unique_ptr<TimerQueue> timerQueue_;
#endif
    ReceiveTimePoint_t receiveTimePoint_;
    ChannelList activeChannels_;

    /* cross-threads wait/notify helper */
#ifdef MUDUO_USE_MEMPOOL
    std::unique_ptr<Bridge, base::deleter_t<Bridge>> bridge_;
#else
    std::unique_ptr<Bridge> bridge_;
#endif
    std::mutex mtx_;    // for sync EventLoop::pendingCbsQueue_
    PendingCallbacksQueue pendingCbsQueue_;
    std::atomic_bool callingPendingCbs_;
};

#ifdef MUDUO_USE_MEMPOOL
    /// Factory pattern
    inline std::unique_ptr<EventLoop, base::deleter_t<EventLoop>> CreateEventLoop() {
        return EventLoop::Create();
    }

    using EventLoopPtr = std::unique_ptr<EventLoop, base::deleter_t<EventLoop>>;
#else
    /// Factory pattern
    inline std::unique_ptr<EventLoop> CreateEventLoop() {
        return EventLoop::Create();
    }

    using EventLoopPtr = std::unique_ptr<EventLoop>;
#endif

} // namespace muduo 

#endif // MUDUO_EVENTLOOP_H