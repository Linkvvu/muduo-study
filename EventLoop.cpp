#include <muduo/poller/PollPoller.h>
#include <muduo/EventLoop.h>
#include <muduo/Callbacks.h>
#include <muduo/TimerQueue.h>
#include <muduo/Channel.h>
#include <muduo/Bridge.h>
#ifdef MUDUO_USE_MEMPOOL
    #include <new>  // emplacement new
#endif
#include <chrono>
#include <cassert>
#include <sys/poll.h>

/// TODO: 为EventLoop的组件提供新版本的构造函数，使其能够在EventLoop实例所持有的内存池中被构建

namespace {
    thread_local muduo::EventLoop* tl_loop_inThisThread = {nullptr};
}

namespace muduo {

using namespace std::chrono;

const EventLoop::TimeoutDuration_t EventLoop::kPollTimeout = duration_cast<EventLoop::TimeoutDuration_t>(seconds(5));

/**
 * @code
 * +--------------------------------------+
 * |              MemoryPool              |
 * |                                      |
 * | +----------------------------------+ |
 * | |        EventLoop instance        | |
 * | +----------------------------------+ |
 * | |   TcpConnection&shared_count     | |
 * | +----------------------------------+ |
 * | |   TcpConnection&shared_count     | |
 * | +----------------------------------+ |
 * | |               ...                | | 
 * | +----------------------------------+ |
 * |                                      |
 * +--------------------------------------+
 * @endcode 
 */
#ifdef MUDUO_USE_MEMPOOL
    std::unique_ptr<EventLoop, base::deleter_t<EventLoop>> EventLoop::Create() {
        static_assert(sizeof(EventLoop) <= base::MemoryPool::kMax_Bytes, "EventLoop实例应当必须被分配在内存池中，而不是被转调用至一级配置器");
        
        // 该内存池生命周期由shared_ptr管理
        auto pool = std::make_shared<base::MemoryPool>();   // create a memory pool instance

        /*
            The EventLoop instance is constructed in the memory pool,
            so can`t use "delete" key to clear the instance,
            just need to call EventLoop::destructor to destroy the object,
            then deallocate the space to allocator
        */ 
        base::deleter_t<EventLoop> deleter = [pool](EventLoop* loop) {  // shared count +1
            loop->~EventLoop();
            pool->deallocate(loop, sizeof (*loop));  // return space to MemoryPool instance

            // when lambda exit, The Instance of memory pool held by EventLoop object will be destroyed (if shared count is 1)
        };

        EventLoop* loop_ptr = nullptr;
        loop_ptr = new (pool.get()) EventLoop(pool);    // Be constructed in the specified memory pool instance
        assert(loop_ptr != nullptr);    // 当构造函数抛出异常，loop_ptr == nullptr

        return std::unique_ptr<EventLoop, base::deleter_t<EventLoop>>(loop_ptr, std::move(deleter)); 
    }

    EventLoop::EventLoop(const std::shared_ptr<base::MemoryPool>& pool)
        : threadId_(::pthread_self())
        , looping_(false)
        , quit_(false)
        , eventHandling_(false)
        , memPool_(pool)
        , poller_(new (memPool_.get()) PollPoller(this), std::bind(&base::DestroyWithMemPool<PollPoller, Poller>, std::placeholders::_1, memPool_.get()))
        , timerQueue_(new (memPool_.get()) TimerQueue(this), std::bind(&base::DestroyWithMemPool<TimerQueue>, std::placeholders::_1, memPool_.get()))
        , activeChannels_(base::allocator<Channel*>(GetMemoryPool()))
        , bridge_(new (memPool_.get()) Bridge(this), std::bind(&base::DestroyWithMemPool<Bridge>, std::placeholders::_1, memPool_.get()))
        , pendingCbsQueue_(base::allocator<PendingEventCb_t>(GetMemoryPool()))   // Use base::allocator to allocate store
        , callingPendingCbs_(false)
    {
        LOG_DEBUG << "EventLoop is created in thread " << threadId_;
        if (tl_loop_inThisThread != nullptr) {
            LOG_FATAL << "Another EventLoop instance " << tl_loop_inThisThread
                    << " exists in this thread";
        } else {
            tl_loop_inThisThread = this;
        }
    }
#else
    std::unique_ptr<EventLoop> EventLoop::Create() {
        return std::unique_ptr<EventLoop>(new EventLoop()); 
    }

    EventLoop::EventLoop()
        : threadId_(::pthread_self())
        , looping_(false)
        , quit_(false)
        , eventHandling_(false)
        , poller_(std::make_unique<detail::PollPoller>(this))
        , timerQueue_(std::make_unique<TimerQueue>(this))
        , activeChannels_()
        , bridge_(std::make_unique<Bridge>(this))
        , pendingCbsQueue_()
        , callingPendingCbs_(false)
    {
        LOG_DEBUG << "EventLoop is created in thread " << threadId_;
        if (tl_loop_inThisThread != nullptr) {
            LOG_FATAL << "Another EventLoop instance " << tl_loop_inThisThread
                    << " exists in this thread";
        } else {
            tl_loop_inThisThread = this;
        }
    }
#endif

EventLoop::~EventLoop() {
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
            << " destructs in thread " << ::pthread_self();
    tl_loop_inThisThread = nullptr;
}

EventLoop* EventLoop::GetCurrentThreadLoop() {
    return tl_loop_inThisThread;
}

void EventLoop::PrintActiveChannels() const {
    for (const Channel* channel : activeChannels_) {
        LOG_TRACE << "<" << channel->REventsToString() << "> ";
    }
}

void EventLoop::Loop() {
    AssertInLoopThread();   // must loop in itself thread
    assert(!looping_);
    assert(!eventHandling_);
    assert(!callingPendingCbs_);

    looping_ = true;
    quit_ = false;

    LOG_TRACE << "EventLoop " << this << " start to loop";

    while (!quit_) {
        activeChannels_.clear();    // Clear the list for polling new channels
        receiveTimePoint_ = poller_->Poll(kPollTimeout, &activeChannels_);
        if (muduo::GetLoglevel() <= Logger::LogLevel::TRACE) {
            PrintActiveChannels();
        }
        HandleActiveChannels();
        HandlePendingCallbacks();
    }
    looping_ = false;
}

void EventLoop::HandleActiveChannels() {
    assert(!eventHandling_);
    eventHandling_ = true;
    for (auto c : activeChannels_) {
        c->HandleEvents(receiveTimePoint_);
    }
    eventHandling_ = false;
}

void EventLoop::UpdateChannel(Channel* c) {
    poller_->UpdateChannel(c);
}

void EventLoop::RemoveChannel(Channel* c) {
    poller_->RemoveChannel(c);
}

TimerId_t EventLoop::RunAt(const TimePoint_t& when, const TimeoutCb_t& cb) {
    return timerQueue_->AddTimer(when, TimeoutDuration_t::zero(), cb);
}

TimerId_t EventLoop::RunAfter(const Interval_t& delay, const TimeoutCb_t& cb) {
    using namespace std;
    auto timepoint = chrono::steady_clock::now() + delay;
    return RunAt(timepoint, cb);
}

TimerId_t EventLoop::RunEvery(const Interval_t& interval, const TimeoutCb_t& cb) {
    using namespace std;
    auto timepoint = chrono::steady_clock::now() + interval;
    return timerQueue_->AddTimer(timepoint, interval, cb);
}

void EventLoop::cancelTimer(const TimerId_t timerId) {
    timerQueue_->CancelTimer(timerId);
}

void EventLoop::EnqueueEventLoop(const PendingEventCb_t& cb) {
    {
        std::lock_guard<std::mutex> guard(mtx_);
        pendingCbsQueue_.push_back(cb);
    }

    /**
     * 1. 如果不在IO线程中，因为IO线程此时可能阻塞在poll中，为确保任务即使被处理，故要调用WakeUp
     * 2. 如果在IO线程中并且此时线程正在处理pending Callbacks，由Loop内部实现决定此时还需调用WakeUp,防止IO线程阻塞
     * 3. 如果在IO线程中且此时线程正在处理activeChannels的callback，则无需调用WakeUp
    */
    if (!IsInLoopThread() || callingPendingCbs_) {
        bridge_->WakeUp();
    }
}

void EventLoop::RunInEventLoop(const PendingEventCb_t& cb) {
    if (IsInLoopThread()) {
        cb();
    } else {
        EnqueueEventLoop(cb);
    }
}

void EventLoop::HandlePendingCallbacks() {
#ifdef MUDUO_USE_MEMPOOL
    std::vector<PendingEventCb_t, base::allocator<PendingEventCb_t>> tmp_queue(GetMemoryPool());
#else
    std::vector<PendingEventCb_t> tmp_queue;
#endif
    callingPendingCbs_.store(true);
    {   // 缩短临界区大小
        std::lock_guard<std::mutex> guard(mtx_);
        tmp_queue.swap(pendingCbsQueue_);    // Can effectively prevent deadlock
    }
    for (const auto& cb : tmp_queue) {
        cb.operator()();
    }
    callingPendingCbs_.store(false);
}

void EventLoop::Quit() {
    assert(quit_ == false);
    quit_.store(true);
    if (!IsInLoopThread()) {
        bridge_->WakeUp();
    }
}

}
