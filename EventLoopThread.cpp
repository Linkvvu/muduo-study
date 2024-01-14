#include <muduo/EventLoop.h>
#include <muduo/EventLoopThread.h>

using namespace muduo;

EventLoopThread::EventLoopThread(const IoThreadInitCallback_t& cb, const std::string& n)
    : loop_(nullptr)
    , name_(n)
    , initCb_(cb)
    , IoThread_(nullptr)
    , isExit_(false)
    , mtx_()
    , cv_()
    { }

EventLoopThread::~EventLoopThread() noexcept {
    isExit_ = true;
    if (loop_ != nullptr) {
        loop_->Quit();  // 通知loop结束循环
        assert(IoThread_->joinable());
        IoThread_->join();
    }
}

EventLoop* EventLoopThread::Run() {
    // TcpServer::ListenAndServe use CAS, So it's no longer needed here
    assert(IoThread_.get() == nullptr);
    
    IoThread_.reset(new std::thread([this]() {
        this->ThreadFunc();
    }));

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
    // EventLoop loop; // create a EventLoop on stack
    EventLoopPtr loop = muduo::CreateEventLoop();
    if (initCb_.operator bool()) {
        initCb_(loop.get());
    }

    {
        std::lock_guard<std::mutex> guard(mtx_);
        if (isExit_ == false) {
            loop_ = loop.get();
        } else {
            cv_.notify_one();
            return;
        }
    }
    cv_.notify_one();

    loop->Loop();   // start loop

    std::lock_guard<std::mutex> guard(mtx_);
    loop_ = nullptr;
}