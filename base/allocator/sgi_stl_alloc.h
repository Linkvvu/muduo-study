#if !defined(MUDUO_BASE_ALLOCATOR_ALLOC_H)
#define MUDUO_BASE_ALLOCATOR_ALLOC_H

#include <muduo/base/allocator/mem_pool.h>
#include <memory>
#include <type_traits>


namespace muduo {
namespace base {
namespace detail {

/// To adapt standard STL allocator
template <typename T, typename Alloc>
class simple_allocator {
public:
    /// The type traits Required by STL Allocator standard
    using value_type = T;
    using propagate_on_container_swap = std::false_type; // The allocator doesn't swap when std::container swap

    simple_allocator() = delete;

    /// 为满足分配器传播机制的构造函数
    template <typename V>
    simple_allocator(const simple_allocator<V, Alloc>& another)
        : alloc_(another.alloc_)
        { }

    /* explicit */ simple_allocator(const std::shared_ptr<Alloc>& alloc) 
            : alloc_(alloc)
        { }
    
    /// copy constructor 
    simple_allocator(const simple_allocator&) = default;
    simple_allocator& operator=(const simple_allocator&) = default;

    /// The interface Required by standard STL Allocator
    /// @param n The number of elements to be allocated
    T* allocate(size_t n)
    { 
        return n == 0 ? nullptr : static_cast<T*>( alloc_->allocate(n * sizeof(T)) ); 
    }

    /// The interface Required by standard STL Allocator
    void deallocate(T* ptr, size_t n)
    {
        if (n != 0) {
            alloc_->deallocate(ptr, n * sizeof(T));
        }
    }

    bool operator ==(simple_allocator<T, Alloc>& other) {
        return this->alloc_ == other.alloc_;
    }

    std::shared_ptr<Alloc> alloc_;  // Aggregation
};

} // namespace detail 

template <typename T>
using allocator = detail::simple_allocator<T, base::MemoryPool>;

} // namespace base 
} // namespace muduo 

#endif // MUDUO_BASE_ALLOCATOR_ALLOC_H
