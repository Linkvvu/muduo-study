#if !defined(MUDUO_BASE_ALLOCATOR_ALLOC_H)
#define MUDUO_BASE_ALLOCATOR_ALLOC_H

#include <muduo/base/allocator/mem_pool.h>
#include <memory>


namespace muduo {
namespace base {
namespace detail {

/// To adapt standard STL allocator
template <typename T, typename Alloc>
class simple_allocator {
public:
    /// The type traits Required by STL Allocator standard
    using value_type = T;

    simple_allocator()
        : alloc_(nullptr)
        {}

    /// 为满足分配器传播机制的构造函数
    template <typename V>
    simple_allocator(const simple_allocator<V, Alloc>& another)
        : alloc_(another.alloc_)
        { }

    simple_allocator(const std::shared_ptr<Alloc>& alloc) 
            : alloc_(alloc)
        { }
    
    /// copy constructor 
    simple_allocator(const simple_allocator&) = default;
    simple_allocator& operator=(const simple_allocator&) = default;

    /// The interface Required by standard STL Allocator
    /// @param n The number of elements to be allocated
    T* allocate(size_t n)
    { 
        if (alloc_)
            return n == 0 ? nullptr : static_cast<T*>( alloc_->allocate(n * sizeof(T)) ); 
        else
            return std::allocator<T>().allocate(n);
    }

    /// The interface Required by standard STL Allocator
    void deallocate(T* ptr, size_t n)
    {
        if (n != 0) {
            if (alloc_)
                alloc_->deallocate(ptr, n * sizeof(T));
            else
                std::allocator<T>().deallocate(ptr, n);
        }
    }
    
    std::shared_ptr<Alloc> alloc_;  // Aggregation
};

} // namespace detail 

template <typename T>
using alloctor = detail::simple_allocator<T, base::MemoryPool>;

} // namespace base 
} // namespace muduo 

#endif // MUDUO_BASE_ALLOCATOR_ALLOC_H
