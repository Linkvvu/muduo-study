#if !defined(MUDUO_BASE_BOUNDED_BLOCKINGQUEUE_H)
#define MUDUO_BASE_BOUNDED_BLOCKINGQUEUE_H

#include <boost/noncopyable.hpp>
#include <muduo/base/condition_variable.h>
#include <list>

namespace muduo {

template <typename T>
class bounded_blockingQueue : private boost::noncopyable {
public:
    bounded_blockingQueue(const size_t cap = 100)
        : mutex_()
        , notEmptyCV_(mutex_)
        , notFullCV_(mutex_)
        , queue_()
        , capacities_(100) {}
    
    void put(const T& item) {
        {
            muduo::mutexLock_guard locker(mutex_);
            while (queue_.size() == capacities_) {
                notFullCV_.wait();
            }
            assert(queue_.size() < capacities_);
            queue_.push_back(item);
        }
        notEmptyCV_.notify_one();
    }

    T take() {
        T front_elem;
        {
            muduo::mutexLock_guard locker(mutex_);
            while (queue_.empty()) {
                notEmptyCV_.wait();
            }
            assert(queue_.empty() == false);
            front_elem = queue_.front();
            queue_.pop_front();
        }
        notFullCV_.notify_one();
        return front_elem;
    }

    // 由于构造函数中对队列reserve了足够的内存
    // 故当size() == capacity()时，判定队列为full
    bool full() const {
        mutexLock_guard locker(mutex_);
        return queue_.size() == capacities_;
    }

    bool empty() const {
        mutexLock_guard locker(mutex_);
        return queue_.size() == 0;
    }

    std::size_t size() const {
        mutexLock_guard locker(mutex_);
        return queue_.size();
    }

    std::size_t capacity() const {
        return capacities_;
    }

private:
    mutable muduo::mutexLock mutex_;
    muduo::condition_variable notEmptyCV_;
    muduo::condition_variable notFullCV_;
    std::list<T> queue_;
    const std::size_t capacities_;
};

} // namespace muduo 


#endif // MUDUO_BASE_BOUNDED_BLOCKINGQUEUE_H
