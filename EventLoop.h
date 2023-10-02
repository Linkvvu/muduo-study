#if !defined(MUDUO_EVENTLOOP_H)
#define MUDUO_EVENTLOOP_H

#include <TimerType.h>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>
#include <iostream>

namespace muduo {
    class TimerQueue;   // forward declaration
    class EventLoop;    // forward declaration
    class Channel;      // forward declaration
    class Poller;       // forward declaration
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
    /// noncopyable  
    EventLoop(const EventLoop&) = delete;

    /// @brief start loop
    void Loop();

    bool IsInLoopThread()
    { return threadId_ == std::this_thread::get_id(); }

    void AssertInLoopThread()
    {
        if (!IsInLoopThread()) { Abort(); }
    }

    // Can be called across-threads
    void Quit() {
        quit_.store(true);
        if (!IsInLoopThread()) {
            // WakeUp
        }
    }

    void UpdateChannel(Channel* c);
    void RemoveChannel(Channel* c);

    TimerId_t RunAt(const TimePoint_t& when, const TimeoutCb_t& cb);
    TimerId_t RunAfter(const Interval_t& delay, const TimeoutCb_t& cb);
    TimerId_t RunEvery(const Interval_t& interval, const TimeoutCb_t& cb);
    void cancelTimer(const TimerId_t timerId);

public:
    static EventLoop& GetCurrentThreadLoop()
    { return *tl_loop_inThisThread; } 

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

    void HandleActiveChannels() const;

private:
    const std::thread::id threadId_;
    bool looping_;
    std::atomic_bool quit_ { false };
    mutable bool eventHandling_;
    std::unique_ptr<Poller> poller_;    // 組合

    ReceiveTimePoint_t receiveTimePoint_;
    using ChannelList_t = std::vector<Channel*>;
    Channel* curChannel_;
    ChannelList_t activeChannels_;
    std::unique_ptr<TimerQueue> timerQueue_;
};
    
} // namespace muduo 

#endif // MUDUO_EVENTLOOP_H