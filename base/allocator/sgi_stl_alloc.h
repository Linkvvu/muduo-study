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

    simple_allocator(Alloc* alloc) 
        : alloc_(alloc)
        { }

    simple_allocator() = delete;

    /// The interface Required by standard STL Allocator
    /// @param n The number of elements to be allocated
    T* allocate(size_t n)
    { return n == 0 ? nullptr : static_cast<T*>( alloc_->allocate(n * sizeof(T)) ); }

    /// The interface Required by standard STL Allocator
    void deallocate(T* ptr, size_t n)
    {
        if (n != 0)
            alloc_->deallocate(ptr, n * sizeof(T));
    }

private:
    Alloc* alloc_;  // Aggregation
};

/* ======================================================================================== */

/// Sub allocator
/// non-copyable
// template <int unused /* , bool thread_safe */>
// class pool_alloc {
// public:
//     explicit pool_alloc()
//         : pool_(std::make_unique<mem_pool>())
//         { }

//     /// @note not thread-safe
//     void* allocate(size_t n) {
//         return pool_->allocate(n);
//     }

//     /// @note not thread-safe
//     void deallocate(void* ptr, size_t n) {
//         pool_->deallocate(ptr, n);
//     }

//     /// Get internal memory pool 
//     mem_pool* GetPool() 
//     { return pool_.get(); }

// private:
//     std::unique_ptr<mem_pool> pool_;
// };

// using sub_alloc = pool_alloc<0>;
// using alloc = base::detail::sub_alloc;

} // namespace detail 

template <typename T>
using alloctor = detail::simple_allocator<T, base::MomoryPool>;

} // namespace base 
} // namespace muduo 

#endif // MUDUO_BASE_ALLOCATOR_ALLOC_H
