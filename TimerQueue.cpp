#include <muduo/TimerQueue.h>
#include <muduo/Timer.h>
#include <memory>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <sys/timerfd.h>

using namespace muduo;

class TimerQueue::TimerMinHeap {
    // non-copyable & non-moveable
    TimerMinHeap(const TimerMinHeap&) = delete;
    TimerMinHeap operator=(const TimerMinHeap) = delete;

#ifdef MUDUO_USE_MEMPOOL
private:
    using TimerList = std::vector<std::unique_ptr<Timer>, base::allocator<std::unique_ptr<Timer>>>;
    /// The index in @c TimerList of Timer, key=detail::TimerId_t value=index
    using TimerMap = std::unordered_map<detail::TimerId_t, std::size_t, std::hash<detail::TimerId_t>, std::equal_to<detail::TimerId_t>, base::allocator<std::pair<const detail::TimerId_t, size_t>>>;
#else
private:
    using TimerList = std::vector<std::unique_ptr<Timer>>;
    using TimerMap = std::unordered_map<detail::TimerId_t, std::size_t>;
#endif

public:
    TimerMinHeap(TimerQueue* q)
        : owner_(q)
#ifdef MUDUO_USE_MEMPOOL
        , timerList_(owner_->Owner()->GetMemoryPool())
        , positions_(owner_->Owner()->GetMemoryPool())
#endif
        { }

    /**
     * @return Whether need update timerfd expiration-TimePoint
    */
    bool Add(std::unique_ptr<Timer>&& t_p) {
        detail::TimerId_t id = t_p->GetId();
        std::size_t idx = timerList_.size();
        timerList_.push_back(std::move(t_p));
        positions_[id] = idx;

        assert(timerList_.size() == positions_.size());

        if (idx == 0) { return true; }

        std::size_t parent_idx = (idx - 1) / 2;
        while (timerList_[idx]->ExpirationTime() < timerList_[parent_idx]->ExpirationTime()) {
            using namespace std;
            swap(timerList_[idx], timerList_[parent_idx]);
            // update positions
            positions_[timerList_[idx]->GetId()] = idx;
            positions_[timerList_[parent_idx]->GetId()] = parent_idx;

            if (parent_idx != 0) {
                idx = parent_idx;
                parent_idx = (idx - 1) / 2;
                continue;
            } else {
                return true;
            }
        }
        return false;
    }

    bool Del(const detail::TimerId_t id) {
        std::size_t idx = -1;   // unsigned LONG maximum
        try {
            idx = positions_.at(id);
        } catch(const std::out_of_range& e) {
            // throw ?
            return false;
        }

        if (idx == 0) {
            this->Pop();
            return true;
        }
        
        detail::TimerId_t backTimer_id = timerList_.back()->GetId();
        using namespace std;
        // tips: the implementation of a standard swap checks the self-swap
        swap(timerList_[idx], timerList_.back());
        timerList_.pop_back();
        auto ret = positions_.erase(id);
        assert(ret == 1); (void)ret;
        assert(timerList_.size() == positions_.size());

        if (backTimer_id != id) {
            // the specified timer is not in the timerList's back, update the new position and adjust the structure 
            positions_[backTimer_id] = idx;
            Adjust(idx);
        }
        return false;
    }

    const std::unique_ptr<Timer>& Top() const {
        return timerList_.front();
    }

    std::unique_ptr<Timer>& Top() {
        return timerList_.front();
    }

    void Adjust(std::size_t cur_mid) {
        while (cur_mid * 2 + 1 < timerList_.size()) {
            std::size_t left = cur_mid * 2 + 1, right = cur_mid * 2 + 2;
            std::size_t candidate = cur_mid; 
            if (left < timerList_.size() && timerList_[left]->ExpirationTime() < timerList_[candidate]->ExpirationTime()) {
                candidate = left;
            }
            if (right < timerList_.size() && timerList_[right]->ExpirationTime() < timerList_[candidate]->ExpirationTime()) {
                candidate = right;
            }

            if (candidate != cur_mid) {   // idx指向的定时器需要向下层调整
                using namespace std;
                // the implementation of a standard swap checks the self-swap
                swap(timerList_[cur_mid], timerList_[candidate]);
                // update positions
                positions_[timerList_[candidate]->GetId()] = candidate;
                positions_[timerList_[cur_mid]->GetId()] = cur_mid;

                cur_mid = candidate;
            } else {    // adjustment is completed
                break;
            }
        }
    }

    bool Empty() const { return timerList_.empty(); } 

    /**
     * call by TimerQueue::GetExpiredTimers
    */
    void PopMovedTimer(const detail::TimerId_t id) {
        assert(!timerList_.empty());

        if (timerList_.size() > 1) {
            assert(positions_.find(id) != positions_.end());
            size_t target_idx = positions_[id];
            bool flag = timerList_.back()->GetId() != id;

            using namespace std;
            // tips: the implementation of a standard swap checks the self-swap
            swap(timerList_[target_idx], timerList_.back());
            timerList_.pop_back();
            auto ret = positions_.erase(id);
            assert(ret == 1); (void)ret;
            assert(timerList_.size() == positions_.size());
            if (flag) {
                // the specified timer is not in the timerList's back, update the new position and adjust the structure 
                positions_[timerList_[target_idx]->GetId()] = target_idx;
                Adjust(target_idx);
            }
        } else if (timerList_.size() == 1) {
            assert(positions_.size() == 1);
            timerList_.clear();
            positions_.clear();
        }
    }

private:
    /**
     * call by TimerMinHeap::Del
    */
    void Pop() {
        PopMovedTimer(timerList_.front()->GetId());
    }



private:
    TimerQueue* const owner_;
    TimerList timerList_;
    TimerMap positions_;
};

/*************************************************************************************************/

TimerQueue::TimerQueue(EventLoop* owner)
    : owner_(owner)
#ifdef MUDUO_USE_MEMPOOL
    , watcher_(new (owner_->GetMemoryPool()) Watcher(this))
#else
    , watcher_(std::make_unique<Watcher>(this))
#endif
    , heap_(std::make_unique<TimerMinHeap>(this))
    , nextTimerId_(0)
    , latestTime_(TimePoint_t::max())
{
    
}

TimerQueue::~TimerQueue() noexcept = default;

detail::TimerId_t TimerQueue::AddTimer(const TimePoint_t& when, const Interval_t& interval, const TimeoutCb_t& cb)
{
    assert(when != TimePoint_t::max());
    // just need to ensuring the atomic
    int cur_timer_id = nextTimerId_.fetch_add(1, std::memory_order::memory_order_relaxed);
    owner_->RunInEventLoop([=]() {  // Capture by value
        std::unique_ptr<Timer> t_p = std::make_unique<Timer>(when, interval, cb, cur_timer_id);
        this->AddTimerInLoop(t_p);
    });
    return cur_timer_id;
}

void TimerQueue::AddTimerInLoop(std::unique_ptr<Timer>& t_p) {
    owner_->AssertInLoopThread();
    auto timeout = t_p->ExpirationTime();
    bool latest_need_update = heap_->Add(std::move(t_p));
    if (latest_need_update) {
        latestTime_ = timeout;
        ResetTimerfd();
    }
}

void TimerQueue::CancelTimer(const detail::TimerId_t id) {
    owner_->RunInEventLoop(std::bind(&TimerQueue::CancelTimerInLoop, this, id));
}

void TimerQueue::CancelTimerInLoop(const detail::TimerId_t id) {
    owner_->AssertInLoopThread();
    bool latest_need_update = heap_->Del(id);
    if (latest_need_update) {
        if (heap_->Empty()) {
            latestTime_ = TimePoint_t::max();
        } else {
            latestTime_ = heap_->Top()->ExpirationTime();
        }
        ResetTimerfd();
    }
}

void TimerQueue::ResetTimerfd() {
    using namespace std;

    struct itimerspec old_t, new_t;
    ::bzero(&new_t, sizeof new_t);
    if (latestTime_ != TimePoint_t::max()) {
        auto dura = latestTime_.time_since_epoch();
        auto sec = chrono::duration_cast<chrono::seconds>(dura);
        auto nsec =  chrono::duration_cast<chrono::nanoseconds>(dura - sec); 
        new_t.it_value.tv_sec = static_cast<decltype(itimerspec::it_value.tv_sec)>(sec.count());
        new_t.it_value.tv_nsec = static_cast<decltype(itimerspec::it_value.tv_nsec)>(nsec.count());
    }

    //when _pioneer = Timer_t::max, new_ts = 0 will disarms the timer
    watcher_->SetTimerfd(&old_t, &new_t);
}

void TimerQueue::HandleExpiredTimers() {
    // owner_->AssertInLoopThread();   // Already asserted in watcher::HandleExpiredTimers
    ExpiredTimerList expired_timers = GetExpiredTimers();
    for (const auto& t : expired_timers) {
        t->Run();
    }
}

TimerQueue::ExpiredTimerList TimerQueue::GetExpiredTimers() {
    ExpiredTimerList result;
    using namespace std;
    // return all expired timers so far
    while (!heap_->Empty() && heap_->Top()->ExpirationTime() <= chrono::steady_clock::now()) {
        result.emplace_back(std::move(heap_->Top()));
        if (result.back()->Repeat()) {
            TimePoint_t next_expiration = TimePoint_t::clock::now() + result.back()->Interval();
            heap_->Top().reset(new Timer(next_expiration, result.back()->Interval(), result.back()->GetPendingCallback(), result.back()->GetId()));
            heap_->Adjust(0);
        } else {
            heap_->PopMovedTimer(result.back()->GetId());
        }
    }

    if (!heap_->Empty()) {
        assert(latestTime_ < heap_->Top()->ExpirationTime());
        latestTime_ = heap_->Top()->ExpirationTime();
    } else { 
        latestTime_ = TimePoint_t::max();
    } 

    ResetTimerfd();

    return result;  // RVO
}
