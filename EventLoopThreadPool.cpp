#include <muduo/EventLoopThreadPool.h>
#include <muduo/EventLoopThread.h>
#include <muduo/EventLoop.h>
#include <cassert>

using namespace muduo;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, const std::string& name)
    : baseLoop_(base_loop)
    , name_(name)
#ifdef MUDUO_USE_MEMPOOL
    , threadPool_(baseLoop_->GetMemoryPool())
    , loops_(baseLoop_->GetMemoryPool())
#else
    , threadPool_()
    , loops_()
#endif
    { assert(baseLoop_ != nullptr); }

EventLoopThreadPool::~EventLoopThreadPool() noexcept = default;

void EventLoopThreadPool::BuildAndRun() {
    assert(!started_);    
    baseLoop_->AssertInLoopThread();


    for (size_t i = 0; i < poolSize_; i++) {
        std::string cur_trd_name = name_+":"+std::to_string(i);
        threadPool_.emplace_back(std::make_unique<EventLoopThread>(initCb_, cur_trd_name));
        loops_.push_back(threadPool_[i]->Run());
    }
    assert(loops_.size() == poolSize_);
    started_ = true;
    
    if (poolSize_ == 0 && initCb_) {
        initCb_(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::GetNextLoop() const {
    baseLoop_->AssertInLoopThread();
    assert(started_);
    
    EventLoop* result = baseLoop_;
    if (!loops_.empty()) {
        // round-robin
        assert(nextLoopIdx_ < poolSize_);
        result = loops_[nextLoopIdx_];
        nextLoopIdx_ += 1;
        if (nextLoopIdx_ >= loops_.size()) {
            nextLoopIdx_ = 0;
        }
    }
    return result;
}