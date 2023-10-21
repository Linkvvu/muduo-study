#if !defined(MUDUO_BASE_MEMORY_POOL_H)
#define MUDUO_BASE_MEMORY_POOL_H
#include <cstdlib>
#include <cstdint>
#include <cstddef>  // offsetof

#ifndef MEM_POOL_MIN_SIZE
    #define MEM_POOL_MIN_SIZE 256
#endif

namespace muduo {
namespace base {

/// 向OS请求一块对齐后的memory
extern void* allocAligned(int alignment, size_t size);
extern void* alloc(size_t s);
extern void free(void* p);

/// This Memory-pool implementation referenced from nginx

class MemPool {
    struct PSmallBlock {
        char* last;
        char* end;
        PSmallBlock* next;
        int failNum;
    };

    struct PLargeBlock;

    explicit MemPool(size_t size);
    MemPool(const MemPool&) = delete;
    MemPool& operator=(const MemPool&) = delete;
    ~MemPool() noexcept;

public:
    static const size_t kDefaultPoolSize /* 16KB */;   
    static const size_t kPoolAlignment   /* 16B */; 
    static const size_t kAlignment /* sizeof(uintptr_t) */;
    static const size_t kMaxAllocFromSize;  /* maximum size of small-block which can be allocated from mempool */

    /// Factory pattern
    static MemPool* CreateMemoryPool(size_t size);
    static void DestroyMemoryPool(MemPool* p);

    /// Return a aligned pointer refering to allocated block
    void* Palloc(size_t s);
    /// same as MemPool::Palloc, but cleared the block
    void* PallocAndClear(size_t s);
    /// free large-block, otherwise do nothing. 
    void Pfree(void* p);
    /// Reset memory pool 
    void PresetPool();

private:
    /// alloc from small-blocks
    void* PallocSmall(size_t s);
    /// alloc a new small-block to block-list And return a pointer refering to specified size area
    void* PallocNewBlock(size_t s);
    /// alloc a large-block And return
    void* PallocLarge(size_t s);
    
private:
    PSmallBlock b_;
    size_t max_;   
    PSmallBlock* current_;
    PLargeBlock* large_;
};

} // namespace base
} // namespace muduo 

#endif // MUDUO_BASE_MEMORY_POOL_H