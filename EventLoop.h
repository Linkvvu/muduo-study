#if !defined(MUDUO_EVENTLOOP_H)
#define MUDUO_EVENTLOOP_H
#include <muduo/base/allocator/mem_pool.h>
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

class EventLoop {
    /// Construct with memory pool, prevent create on stack 
    EventLoop(base::MomoryPool* pool);
    /// noncopyable & nonmoveable
    EventLoop(const EventLoop&) = delete;
    
public:
    using ReceiveTimePoint_t = std::chrono::system_clock::time_point;
    using TimeoutDuration_t = std::chrono::milliseconds;
    
private:
    static const TimeoutDuration_t kPollTimeout;

public:
    /// Use muduo::base::MemoryPool to alloacte store space 
    /// @note The method will hide the global operator new for this class
    static void* operator new(size_t size, base::MomoryPool* pool);

    /// @brief The method corresponds to @c EventLoop::operator new(size_t size, base::MomoryPool* pool),
    /// Only will be invoked When EventLoop::constructors throws a excepction
    /// @note The method will hide the global operator delete for this class
    static void operator delete(void* p, base::MomoryPool* pool);

    /// @brief 由内存池构造的EventLoop的实例的"删除器"类型
    using deleter_t = std::function<void(EventLoop* loop)>; 

    /// Explicitly declare @c EventLoop::operator new(), uses the global new operator
    static void* operator new(size_t size)
    { return ::operator new(size); }

    /// Explicitly declare @c EventLoop::operator delete(), uses the global delete operator
    static void operator delete(void* p)
    { ::operator delete(p); }
    
    /// @brief Factory pattern
    /// @return A EventLoop instance within muduo::base::memory_pool 
    static std::unique_ptr<EventLoop, EventLoop::deleter_t> Create();

    /// @brief Default constructor
    /// @return A EventLoop instance, don't use muduo::base::memory_pool 
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
    
    base::MomoryPool* GetMemoryPool() {
        return memPool_;
    }

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
    base::MomoryPool* memPool_;  // Loop-level memory pool hanlde

    const pthread_t threadId_;
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
    
/// Factory pattern
inline std::unique_ptr<EventLoop, EventLoop::deleter_t> CreateEventLoop() {
    return EventLoop::Create();
}

} // namespace muduo 

#endif // MUDUO_EVENTLOOP_H