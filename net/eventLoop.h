#if !defined(MUDUO_NET_TCPSERVER_H)
#define MUDUO_NET_TCPSERVER_H

#include <muduo/base/currentThread.h>
#include <muduo/base/timeStamp.h>
#include <boost/noncopyable.hpp>
#include <unistd.h>
#include <vector>
#include <memory>

namespace muduo {
namespace net {

class channel;  // forward declaration
class poller;   // forward declaration

/// @brief Reactor, at most one per thread
class event_loop : private boost::noncopyable {
public:
    event_loop();
    ~event_loop();

    /// @brief loop forever, be called in the same thread as creation of the object
    void loop();

    bool is_inLoop_thread() const { return muduo::currentThread::tid() == holdThreadId_; }

    void assert_loop_in_hold_thread() {
        if (is_inLoop_thread() == false) {
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
        if (is_inLoop_thread() == false) {
            // wakeup();    TODO:唤醒循环
        }
    }

private:
    void abort_for_not_in_holdThread();
    
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
}; 

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_TCPSERVER_H
