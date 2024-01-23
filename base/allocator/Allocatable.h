#if !defined(MUDUO_BASE_ALLOCATOR_ALLOCATABLE_H)
#define MUDUO_BASE_ALLOCATOR_ALLOCATABLE_H

#include <muduo/base/allocator/mem_pool.h>

#ifdef MUDUO_USE_MEMPOOL

#include <muduo/base/Logging.h>

namespace muduo {
namespace base {
namespace detail {

struct Allocatable {
    /// @brief Use specified @c muduo::base::MemoryPool to allocate the storage 
    /// @note The method will hide the global operator new for this class
    static void* operator new(size_t size, base::MemoryPool* pool) {
        return pool->allocate(size);
    };

    /// @brief The method corresponds to @c operator new(size_t size, base::MemoryPool* pool),
    /// Only will be invoked When the key @c new throws a exception by C++ Runtime System
    static void operator delete(void* p, base::MemoryPool* pool) {
        /// FIXME: use allocate-with-cookie OR CRTP to implement
        ///     pool->deallocate(p, sizeof(Derive));
        LOG_FATAL << "Unexpected invoke operator delete(void* p, base::MemoryPool* pool)";
    }

    /// @brief Explicitly delete the normal class-specific @c operator-new
    static void* operator new(size_t size) = delete;

    /// @brief Use Loop-level mempool to deallocate the storage 
    /// @note Must be invoked in the thread where the parameter @c ptr was allocated, for thread-safe
    static void operator delete(void* ptr, size_t size) {
        MemoryPool::GetCurrentThreadMempool()->deallocate(ptr, size);
    }
};

} // namespace detail 
} // namespace base
} // namespace muduo 


#endif

#endif // MUDUO_BASE_ALLOCATOR_ALLOCATABLE_H
