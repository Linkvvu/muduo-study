#if !defined(MUDUO_NET_EVENTLOOPTHREAD_H)
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include <muduo/base/Thread.h>
#include <muduo/base/mutex.h>
#include <muduo/base/Types.h>
#include <muduo/base/condition_variable.h>
#include <muduo/net/callBacks.h>

namespace muduo {
namespace net {

class event_loop;   // forward declaration

class eventLoop_thread : private boost::noncopyable {
public:
    eventLoop_thread(const IO_thread_initializeCallback_t& func = nullptr, const string& thread_name = nullptr);
    ~eventLoop_thread();
    event_loop* start_loop();
    string name() const { return thread_.name(); }
    
private:
    void threadFunc();

private:
    Thread  thread_;    // IO thread
    event_loop* loop_;    // IO thread`s handle of event loop
    bool exiting_;
    mutexLock mutex_;
    condition_variable cv_; // cv for loop instance creation
    IO_thread_initializeCallback_t initFunc_; // 初始化函数
};

} // namespace net 
} // namespace muduo 
#endif // MUDUO_NET_EVENTLOOPTHREAD_H
