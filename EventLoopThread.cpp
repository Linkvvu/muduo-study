#include <EventLoop.h>
#include <EventLoopThread.h>
using namespace muduo;

EventLoopThread::EventLoopThread(const IoThreadInitCb_t& cb, const std::string& n)
    : loop_(nullptr)
    , name_(n)
    , initCb_(cb)
    , IoThread_()
    , isExit_(false)
    , mtx_()
    , cv_()
    { }

EventLoopThread::~EventLoopThread() noexcept {
    std::lock_guard<std::mutex> guard(mtx_);
    isExit_ = true;
    if (loop_ != nullptr) {
        loop_->Quit();  // 通知loop结束循环
    }
}

EventLoop* EventLoopThread::Run() {
    // TODO: use atomic_flag(CAS) for secure multiple calls
    assert(IoThread_.joinable() == false);
    
    std::thread tmp(std::bind(&EventLoopThread::ThreadFunc, this));
    IoThread_.swap(tmp);
    assert(IoThread_.joinable());
    IoThread_.detach();

    EventLoop* res = nullptr;
    {
        std::unique_lock<std::mutex> guard(mtx_);
        while (loop_ == nullptr && !isExit_) {
            cv_.wait(guard);
        }
        res = loop_;
    }
    return res;
}

void EventLoopThread::ThreadFunc() {
    EventLoop loop; // create a EventLoop on stack

    if (initCb_.operator bool()) {
        initCb_(&loop);
    }

    {
        std::lock_guard<std::mutex> guard(mtx_);
        if (isExit_ == false) {
            loop_ = &loop;
        } else {
            cv_.notify_one();
            return;
        }
    }
    cv_.notify_one();

    loop.Loop();   // start loop

    std::lock_guard<std::mutex> guard(mtx_);
    loop_ = nullptr;
}