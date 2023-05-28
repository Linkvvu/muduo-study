#include <muduo/net/eventLoop_threadPool.h>
#include <muduo/net/eventLoopThread.h>
#include <muduo/net/eventLoop.h>
#include <muduo/base/logging.h>

using namespace muduo;
using namespace muduo::net;

eventLoop_threadPool::eventLoop_threadPool(event_loop* const base_loop, const int thread_num, const IO_thread_initializeCallback_t& func)
    : baseLoop_(base_loop)
    , threadNum_(thread_num)
    , nextLoopIdx_(0)
    , threads_()
    , loops_()
    , started_(false)
    , initializeCb_(func) {
        threads_.resize(threadNum_);
        loops_.resize(threadNum_);
    }

eventLoop_threadPool::~eventLoop_threadPool() {
    // do nothing, loop instance is stack variable
}

void eventLoop_threadPool::start() {
    baseLoop_->assert_loop_in_hold_thread();    // must start in the baseLoop`s thread
    assert(started_ == false);
    started_ = true;

    for (int i = 0; i < threadNum_; i++) {
        char buf[32] = {0};
        ::snprintf(buf, sizeof buf, "eventLoop-thread<%d>", i);
        threads_[i].reset(new eventLoop_thread(initializeCb_, buf));
    }

    for (int i = 0; i < threadNum_; i++) {
        loops_[i] = threads_[i]->start_loop();
        LOG_INFO << "The I/O-thread [" << threads_[i]->name() << "] has been started";
    }

    if (threadNum_ == 0 && initializeCb_.operator bool()) {
        initializeCb_(baseLoop_);
    }
}

event_loop* eventLoop_threadPool::get_next_loop() {
    baseLoop_->assert_loop_in_hold_thread();    // must be call in the baseLoop`s thread
    event_loop* loop = baseLoop_;

    if (!loops_.empty()) {
        loop = loops_[nextLoopIdx_++];

        if (nextLoopIdx_ >= threadNum_) {
            nextLoopIdx_ = 0;
        }
    }
    
    return loop;
}