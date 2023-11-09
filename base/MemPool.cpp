#include <base/MemPool.h>
#include <base/Logging.h>
#include <new>  // placement new
#include <cstring>
#include <cassert>
#include <unistd.h>

using namespace muduo::base;

struct MemPool::PLargeBlock {
    void* alloc;
    PLargeBlock* next;
};

static void* alignPtr(void* p, size_t alignment) {
    return (void*) (((uintptr_t) (p) + ((uintptr_t) alignment - 1)) & ~((uintptr_t) alignment - 1));
}

void* muduo::base::allocAligned(int alignment, size_t size) {
    return std::aligned_alloc(alignment, size);
}

void* muduo::base::alloc(size_t s) {
    return std::malloc(s);
}

void muduo::base::free(void* p) {
    std::free(p);
}

/// define static member datas
const size_t MemPool::kDefaultPoolSize = 16*1024;   
const size_t MemPool::kPoolAlignment = 16;
const size_t MemPool::kAlignment = sizeof(uintptr_t);   // align by pointer size
const size_t MemPool::kMaxAllocFromSize = static_cast<size_t>(::getpagesize() - 1);

MemPool* MemPool::CreateMemoryPool(size_t size) {
    if (size < MEM_POOL_MIN_SIZE) {
        LOG_FATAL << "Specified memory-pool size is too small, MEM_POOL_MIN_SIZE = " << MEM_POOL_MIN_SIZE;
    }

    void* p = allocAligned(kPoolAlignment, size);
    MemPool* pool = new (p) MemPool(size);
    assert(p == pool);
    return pool;
}

void muduo::base::MemPool::DestroyMemoryPool(MemPool* p) {
    p->~MemPool();  // manual call destructor
}

MemPool::MemPool(size_t size) {
    static_assert(offsetof(MemPool, MemPool::b_) == 0, "PSmallBlock offset isn`t 0 in MemPool");
    /* init member b_ */
    PSmallBlock* sm = new (this) PSmallBlock();
    sm->end = static_cast<char*>(static_cast<void*>(this)) + size;
    sm->last = static_cast<char*>(static_cast<void*>(this)) + sizeof(MemPool);
    sm->next = nullptr;
    sm->failNum = 0;
    /* init other members */
    size_t available_size = size - sizeof(MemPool);
    max_ = (available_size <= kMaxAllocFromSize) ? available_size : kMaxAllocFromSize;
    current_ = static_cast<PSmallBlock*>(static_cast<void*>(this));
    large_ = nullptr;
}

void* muduo::base::MemPool::Palloc(size_t s) {
    if (s <= max_) {
        return PallocSmall(s);
    }
    return PallocLarge(s);
}

void* MemPool::PallocAndClear(size_t s) {
    void* p = Palloc(s);
    if (p) {
        std::memset(p, 0, s);
        return p;
    }
    return nullptr;
}

void* MemPool::PallocSmall(size_t s) {
    PSmallBlock* cur_block = this->current_;
    do {
        char* m = static_cast<char*>(alignPtr(cur_block->last, kAlignment)); // align pointer
        /**
         * FIXME:
         * may be unsafe? Maybe 'm' is greater than 'cur_block->end' after alignment
         * @code {.cpp}
         * ...
         * if (!(m > cur_block->end)) {
         *      size_t available_size = cur_block->end - m;
         *      if (s <= available_size) {
         *          cur_block->last = m + s;
         *          return m;
         *      }
         * }
         * cur_block = cur_block->next;
         * ...
         * @endcode
         * 
        */
        size_t available_size = cur_block->end - m;
        if (s <= available_size) {
            cur_block->last = m + s;
            return m;
        }
        cur_block = cur_block->next;
    } while(cur_block);
    
    return PallocNewBlock(s);
}

void* MemPool::PallocNewBlock(size_t s) {
    size_t pool_size = this->b_.end - static_cast<char*>(static_cast<void*>(this));    // total size of pool`s block
    void* mem = allocAligned(kPoolAlignment, pool_size);
    if (mem == nullptr) {
        return nullptr;
    }

    /* just alloc a PSmallBlock instead of MemPool */
    PSmallBlock* new_block = new (mem) PSmallBlock;
    new_block->end = static_cast<char*>(mem) + pool_size;
    new_block->next = nullptr;
    new_block->failNum = 0;
    new_block->last = static_cast<char*>(mem) + sizeof(PSmallBlock);

    /* Prepare a area for the client */
    void* res = alignPtr(new_block->last, kAlignment);
    /// FIXME: unsafe!
    /// new_block->last = static_cast<char*>(res) + s;

    /// it`s safe now
    static_assert(sizeof(MemPool) - sizeof(PSmallBlock) >= kAlignment - 1, "A serious error in MemPool internal.");
    new_block->last = static_cast<char*>(res) + s;

    PSmallBlock* b = nullptr;
    for (b = this->current_; b->next; b = b->next) {
        if (b->failNum++ >= 4) {    // if failed to alloc 5 times, set it as invalid
            this->current_ = b->next;
        }
    }

    assert(b->next == nullptr);
    b->next = new_block;

    return res;
}

void* MemPool::PallocLarge(size_t s) {
    void* p = base::alloc(s);
    if (p == nullptr) {
        return nullptr;
    }

    int search_count = 0; 
    for (PLargeBlock* l = this->large_; l != nullptr; l = l->next) {
        if (l->alloc == nullptr) {
            l->alloc = p;
            return p;
        }
        if (search_count++ > 3) {   // for efficiency
            break;
        }
    }

    PLargeBlock* new_large = static_cast<PLargeBlock*>(PallocSmall(sizeof(PLargeBlock)));
    if (new_large == nullptr) {
        base::free(p);
        return nullptr;
    }

    new_large->alloc = p;
    new_large->next = this->large_;
    this->large_ = new_large;

    return p;
}

void MemPool::Pfree(void* p) {
    for (PLargeBlock* l = this->large_; l != nullptr; l = l->next) {
        if (p == l->alloc) {
            base::free(p);
            l->alloc = nullptr;
        }
    }
}

void MemPool::PresetPool() {
    /* free all large-blocks */
    for (PLargeBlock* l; l != nullptr; l = l->next) {
        if (l->alloc) {
            base::free(l->alloc);
        }
    }
    /* reset all small-blocks */
    this->b_.last = static_cast<char*>(static_cast<void*>(this)) + sizeof(MemPool);
    this->b_.failNum = 0;
    for (PSmallBlock* b = this->b_.next; b != nullptr; b = b->next) {
        b->last = static_cast<char*>(static_cast<void*>(b)) + sizeof(PSmallBlock);
        b->failNum = 0;
    }
    /* reset other infos of pool */
    this->current_ = &this->b_;
    assert(static_cast<void*>(this->current_) == static_cast<void*>(this));
    this->large_ = nullptr;
}

MemPool::~MemPool() noexcept {
    /* free all large-blocks */
    for (PLargeBlock* l = this->large_; l; l = l->next) {
        if (l->alloc) {
            base::free(l->alloc);
        }
    }

    /* free all small-blocks*/
    for (PSmallBlock* b = &this->b_, * n = this->b_.next; /* void */; b = n, n = b->next) {
        base::free(b);

        if (n == nullptr) {
            break;
        }
    }
}
