#if !defined(MUDUO_NET_EVENTLOOP_THREADPOOL_H)
#define MUDUO_NET_EVENTLOOP_THREADPOOL_H

#include <boost/noncopyable.hpp>
#include <muduo/net/callBacks.h>
#include <muduo/net/eventLoopThread.h>
#include <memory>
#include <atomic>
#include <vector>

namespace muduo {
namespace net {

class event_loop;       // forward declaration
class eventLoop_thread; // forward declaration
/// @brief IO-thread pool
class eventLoop_threadPool : boost::noncopyable {
public:
    eventLoop_threadPool(event_loop* const base_loop, const int thread_num, const IO_thread_initializeCallback_t& func = nullptr);
    ~eventLoop_threadPool();

    void start();
    bool started() const { return started_; }
    event_loop* get_next_loop();
    
private:
    event_loop* const baseLoop_;                            // main-reactor event_loop
    const int threadNum_;
    int nextLoopIdx_;
    std::vector<std::unique_ptr<eventLoop_thread>> threads_;    // thread-pool
    std::vector<event_loop*> loops_;                        // Loop instance for each thread
    std::atomic<bool> started_;
    IO_thread_initializeCallback_t  initializeCb_;
};

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_EVENTLOOP_THREADPOOL_H
