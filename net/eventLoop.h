#if !defined(MUDUO_NET_EVENTLOOP_H)
#define MUDUO_NET_EVENTLOOP_H

#include <muduo/base/currentThread.h>
#include <muduo/base/timeStamp.h>
#include <muduo/base/mutex.h>
#include <muduo/net/callBacks.h>
#include <boost/noncopyable.hpp>
#include <unistd.h>
#include <atomic>
#include <vector>
#include <memory>

namespace muduo {
namespace net {

class channel;      // forward declaration
class poller;       // forward declaration
class timer_id;     // forward declaration
class timer_queue;  // forward declaration

/// @brief Reactor, at most one per thread
class event_loop : private boost::noncopyable {
public:
    event_loop();
    ~event_loop();

    /// @brief loop forever, be called in the same thread who creation of the object
    void loop();

    bool is_in_eventLoop_thread() const { return muduo::currentThread::tid() == holdThreadId_; }

    void assert_loop_in_hold_thread() {
        if (is_in_eventLoop_thread() == false) {
            abort_for_not_in_holdThread();
        }
    }

    void updateChannel(channel* channel);
    void removeChannel(channel* channel);

    static event_loop* getEventLoopOfCurrentThread();

    void printActiveChannels() const;

    // 该函数可以跨线程调用(不光是只能在本loop对象所属的IO线程调用)
    void quit() {
        quit_ = true;
        if (is_in_eventLoop_thread() == false) {
            wakeup();
        }
    }

    /// @brief invoke target task, I`s async for the client when current thread is`t I/O thread
    timer_id run_at(const TimeStamp& time, const timerCallback_t& func);
    /// @brief invoke target task, I`s async for the client when current thread is`t I/O thread
    timer_id run_after(double delay, const timerCallback_t& func);
    /// @brief invoke target task, I`s async for the client when current thread is`t I/O thread
    /// @note Periodic invoke target task from now on
    timer_id run_every(double interval, const timerCallback_t& func);
    void cancel(timer_id t);

    void wakeup();

    void enqueue_eventLoop(const pendingCallback_t& func);
    void handle_pendingFunctors();
    void run_in_eventLoop_thread(const pendingCallback_t& func);

private:
    void abort_for_not_in_holdThread();
    void handle_wakeupFd_read();
    
private:
    using channelList_t = std::vector<channel*>;

    // Linux平台下的bool类型本身就是atomic操作的
    bool looping_;          // atomic，当前是否处于事件循环
    bool quit_;             // atomic，是否退出事件循环
    bool eventHandling_;    // atomic，是否正在处理事件
    const pid_t holdThreadId_;
    TimeStamp pollReturn_timeStamp_;
    std::unique_ptr<poller> poller_;    // 组合关系, 通过指针实现多态
    channelList_t activeChannles_;       // poller返回的活动通道
    channel* cur_activeChannel_;         // 当前正在处理的活动通道 

    std::unique_ptr<timer_queue> timers_;

    std::unique_ptr<channel> wakeupFdChannel_;
    std::atomic<bool> handlingPendingFunctors_;
    muduo::mutexLock mutex_;    // for protect pendingFunctorsQueue
    std::vector<pendingCallback_t> pendingFunctorQueue_;
}; 

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_EVENTLOOP_H
