#include <muduo/net/eventLoopThread.h>
#include <muduo/net/eventLoop.h>

namespace muduo {
namespace net {

eventLoop_thread::eventLoop_thread(const initializeCallback_t& func, const string& thread_name)
    : thread_([this]() { this->threadFunc(); }, thread_name)
    , loop_(nullptr)
    , exiting_(false)
    , mutex_()
    , cv_(mutex_)
    , initFunc_(func) {}

eventLoop_thread::~eventLoop_thread() {
    exiting_ = true;
    {
        mutexLock_guard locker(mutex_);
        if (loop_ != nullptr) {
            loop_->quit();
        }
    }
   
    if (thread_.started()) {
        thread_.join();
    }
}

event_loop* eventLoop_thread::start_loop() {
    assert(!thread_.started());
    thread_.start();

    event_loop* loop = nullptr;
    {
        mutexLock_guard locker(mutex_);
        while (loop_ == nullptr) {
            cv_.wait();
        }
        loop = loop_;
    }
    return loop;
}

void eventLoop_thread::threadFunc() {
    event_loop loop;
    if (initFunc_) { initFunc_(&loop); }

    {
        mutexLock_guard locker(mutex_);
        loop_ = &loop;
        cv_.notify_one();
    }

    loop.loop();    // start the loop

    mutexLock_guard locker(mutex_);
    loop_ = nullptr;
}

} // namespace net 
} // namespace muduo 