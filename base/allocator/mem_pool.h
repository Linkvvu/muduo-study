#if !defined(MUDUO_BASE_ALLOCATOR_MEM_POOL_H)
#define MUDUO_BASE_ALLOCATOR_MEM_POOL_H

#include <functional>
#include <cassert>
#include <cstdlib>
#include <list>
#include <new>

namespace muduo {
namespace base {
namespace detail {

/// First-level allocator  
template <int unused>
class malloc_alloc {
private:
    static void* oom_malloc(size_t n);
    using oom_handler_t = void* (*)(); 
    static oom_handler_t mmo_handler;

public:
    /// @param n Need allocated size
    /// @note When there is not enough memory to allocate, @c abort()
    static void* allocate(size_t n) {
        void* res = ::malloc(n);
        if (res == nullptr)
            res = oom_malloc(n);
        return res;
    }

    static void deallocate(void* ptr, size_t /* n */)
    { ::free(ptr); }

    static oom_handler_t set_malloc_handler(oom_handler_t handler) {
        oom_handler_t old = malloc_alloc<unused>::mmo_handler;
        malloc_alloc<unused>::mmo_handler = handler;
        return old;
    }
};


/// define initial value
template <int unused>
typename malloc_alloc<unused>::oom_handler_t malloc_alloc<unused>::mmo_handler = nullptr;

template <int unused>
void* malloc_alloc<unused>::oom_malloc(size_t n) {
    oom_handler_t handler;
    void* result;

    while (true) {
        handler = malloc_alloc<unused>::mmo_handler;
        if (handler == nullptr) {
            throw std::bad_exception();
        }

        handler();
        result = ::malloc(n);
        if (result) {
            return result;
        }
    }
}

/// Instantiate class template @c malloc_alloc
using master_alloc = malloc_alloc<0>;

/* ======================================================================================== */

/// @note All the operations are not thread-safe
/// non-copyable & non-moveable(might move, but hasn't implement)
class mem_pool {
    mem_pool(const mem_pool&) = delete;
    mem_pool& operator=(const mem_pool&) = delete;

private:
    union obj
    {
        union obj* next;
        char client_data[1];    /* The client sees this */
    };

public:
    static const size_t kALIGN = 8;
    static const size_t kMax_Bytes = 512;
    static const size_t kFreeListsNum = kMax_Bytes / kALIGN;

    static_assert(kMax_Bytes % kALIGN == 0, "mem_pool::kMaxBytes must be a multiple of mem_pool::kALIGN");

private:
    /// 将 @c bytes 上调至 @c kALIGN 的倍数 
    static size_t ROUND_UP(size_t bytes)
    { return (((bytes) + kALIGN-1) & ~(kALIGN - 1)); }

    /// 根据区块的大小 @c bytes 找到目标free_list的索引
    static size_t GET_FREELIST_INDEX(size_t bytes)
    { return (((bytes) + kALIGN-1) / kALIGN - 1); }

public:
    mem_pool() = default;

    void* allocate(size_t n) {
        if (n > static_cast<size_t>(kMax_Bytes)) {
            return detail::master_alloc::allocate(n);
        }

        list_header_t target_list = nullptr;
        obj* result;

        target_list = free_lists + GET_FREELIST_INDEX(n);    // Get target list
        result = *target_list;
         
        if (result == nullptr) {    // The list has`t available space
            void* r = refill(ROUND_UP(n));
            return r;
        }

        *target_list = result->next;  // Adjust current list 
        return result;
    }

    void deallocate(void* ptr, size_t n) {
        if (n > static_cast<size_t>(kMax_Bytes)) {
            detail::master_alloc::deallocate(ptr, n);
            return;
        }

        obj* recycle = static_cast<obj*>(ptr);
        list_header_t target_list = nullptr;

        target_list = free_lists + GET_FREELIST_INDEX(n);
        recycle->next = *target_list;
        *target_list = recycle;
    }

    /// 向操作系统归还已分配的内存区域
    ~mem_pool() noexcept {
        for (void* p : allocated_area) {
            ::free(p);
        }
        allocated_area.clear();
    }

private:
    /// @brief allocate a chunk that can accommodate @c n_objs @c sub_alloc<unused>::obj of size @c size.
    /// @note size 必须是 @c mem_pool::kALIGN 的倍数
    char* chunk_alloc(size_t size, int* n_objs);

    /// @brief Fills new chunk to target list 
    /// @param n Chunk size, determines which list to fill
    /// @return A chunk of size @c n
    /// @note n 必须是 @c mem_pool::kALIGN 的倍数
    void* refill(size_t n);

private:
    using list_header_t = obj* volatile *; 

    obj* volatile free_lists[kFreeListsNum] {};
    char* begin_free {nullptr};
    char* end_free {nullptr};
    size_t heap_size {0};

    std::list<void*> allocated_area;
};

} // namespace detail 

using MemoryPool = detail::mem_pool;

/// @brief 由内存池构造的实例的"删除器"类型, 为适配于智能制造
template <typename T>
using deleter_t = std::function<void(T* p)>; 

} // namespace base 
} // namespace muduo 

#endif // MUDUO_BASE_ALLOCATOR_MEM_POOL_H
