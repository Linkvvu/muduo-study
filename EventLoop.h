#if !defined(MUDUO_EVENTLOOP_H)
#define MUDUO_EVENTLOOP_H

#include <thread>
#include <iostream>


namespace muduo {
    class EventLoop;    // forward declaration
}

namespace {
    extern thread_local muduo::EventLoop* tl_loop_inThisThread;
} // namespace 

namespace muduo {

class EventLoop {
public:
    EventLoop();

    /// noncopyable  
    EventLoop(const EventLoop&) = delete;

    /// @brief start loop
    void Loop();

    bool IsInLoopThread()
    { return threadId_ == std::this_thread::get_id(); }

    void AssertInLoopThread() {
        if (!IsInLoopThread()) {
            Abort();
        }
    }

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

private:
    const std::thread::id threadId_;
    bool looping_;
};
    
} // namespace muduo 

#endif // MUDUO_EVENTLOOP_H