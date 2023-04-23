#if !defined(MUDUO_NET_TCPSERVER_H)
#define MUDUO_NET_TCPSERVER_H

#include <muduo/base/currentThread.h>
#include <boost/noncopyable.hpp>
#include <unistd.h>

namespace muduo {
namespace net {

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

    static event_loop* getEventLoopOfCurrentThread();


private:
    void abort_for_not_in_holdThread();
    
private:
    bool looping_; // atomic
    const pid_t holdThreadId_;
}; 

} // namespace net 
} // namespace muduo 

#endif // MUDUO_NET_TCPSERVER_H
