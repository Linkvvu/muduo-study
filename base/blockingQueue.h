#if !defined(MUDUO_BASE_BLOCKINGQUEUE_H)
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include <boost/noncopyable.hpp>
#include <muduo/base/condition_variable.h>
#include <deque>

namespace muduo {

template<typename T>
class blockingQueue : private boost::noncopyable {
public:
    explicit blockingQueue()
        : mutex_()
        , notEmptyCV_(mutex_)
        , queue_() {}

    void put(const T& item) {
        {
            mutexLock_guard locker(mutex_);
            queue_.push_back(item);
        }
        notEmptyCV_.notify_one();
    }

    void put(T&& item) {
        {
            mutexLock_guard locker(mutex_);
            queue_.push_back(item);
        }
        notEmptyCV_.notify_one();
    }

    T take() {
        T front_elem;
        {
            mutexLock_guard locker(mutex_);
            while (queue_.empty()) {
                notEmptyCV_.wait();
            }
            assert(queue_.empty() != true);
            front_elem = queue_.front();
            queue_.pop_front();
        }
        return front_elem;
    }

    size_t size() const {
        mutexLock_guard locker(mutex_);
        return queue_.size();
    }

private:
    mutable muduo::mutexLock mutex_;
    muduo::condition_variable notEmptyCV_;
    std::deque<T> queue_;
};

} // namespace muduo 

#endif // MUDUO_BASE_BLOCKINGQUEUE_H
