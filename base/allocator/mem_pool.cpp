#include <muduo/base/allocator/mem_pool.h>

#ifdef MUDUO_USE_MEMPOOL

#include <muduo/base/Logging.h>
#include <muduo/EventLoop.h>

using namespace muduo::base::detail;

namespace {
    thread_local muduo::base::MemoryPool* tl_mempool_inThisThread = {nullptr};
} // namespace 

mem_pool::mem_pool(EventLoop* loop)
    : loop_(loop)
{
    if (tl_mempool_inThisThread != nullptr) {
        LOG_FATAL << "Another Mempool instance " << tl_mempool_inThisThread
                << " exists in this thread";
    } else {
        tl_mempool_inThisThread = this;
    }
}

muduo::base::detail::mem_pool::~mem_pool() {
    loop_->AssertInLoopThread();

    for (void* p : allocated_area) {
        ::free(p);
    }
    allocated_area.clear();
    tl_mempool_inThisThread = nullptr;
}

mem_pool* mem_pool::GetCurrentThreadMempool() {
    return tl_mempool_inThisThread;
}

void* mem_pool::allocate(size_t n) {
    if (n > static_cast<size_t>(kMax_Bytes)) {
        return detail::master_alloc::allocate(n);
    }

    loop_->AssertInLoopThread();

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

void mem_pool::deallocate(void* ptr, size_t n) {
    if (n > static_cast<size_t>(kMax_Bytes)) {
        detail::master_alloc::deallocate(ptr, n);
        return;
    }
    
    loop_->AssertInLoopThread();

    obj* recycle = static_cast<obj*>(ptr);
    list_header_t target_list = nullptr;

    target_list = free_lists + GET_FREELIST_INDEX(n);
    recycle->next = *target_list;
    *target_list = recycle;
}

void* mem_pool::refill(size_t n) {
    int n_objs = 20;
    char* chunk = chunk_alloc(n, &n_objs);

    if (n_objs == 1) {
        return chunk;
    }

    list_header_t target_list = free_lists + GET_FREELIST_INDEX(n);
    obj* result = static_cast<obj*>(static_cast<void*>(chunk));
    obj* cur_obj = static_cast<obj*>(static_cast<void*>(chunk + n));
    *target_list = cur_obj;
    obj* next_obj = nullptr;

    // 尾插法
    for (int i = 1; ; i++) {
        next_obj = static_cast<obj*>(static_cast<void*>((char*) cur_obj + n));
        if (n_objs - 1 == i) {
            cur_obj->next = nullptr;
            break;
        } else {
            cur_obj->next = next_obj;
        }
        cur_obj = next_obj;
    }
    return result;
}

char* mem_pool::chunk_alloc(size_t size, int* n_objs) {
    size_t total_size = size * (*n_objs);
    size_t left_size = end_free - begin_free;
    char* result = nullptr;

    if (left_size >= total_size) {  // 内存池剩余空间满足需求量
        result = begin_free;
        begin_free += total_size;
        return result;
    } else if (left_size >= size) { // 内存池空间不完全满足需求量，但足够供应一个(含)以上的chunk
        *n_objs = left_size / size;
        total_size = size * (*n_objs);
        result = begin_free;
        begin_free += total_size;
        return result;
    } else {    // 内存池连一块chunk都无法提供
        if (left_size > 0) {    // 内存池内还有残余空间，将其编入合适的list中
            assert(left_size % kALIGN == 0);    // 断言剩余的内存大小一定是kALIGN的整数倍
            list_header_t target_list = free_lists + GET_FREELIST_INDEX(left_size);
            static_cast<obj*>(static_cast<void*>(begin_free))->next = *target_list;
            *target_list = static_cast<obj*>(static_cast<void*>(begin_free));
        }

        size_t bytes_to_get = 2 * total_size + ROUND_UP(heap_size >> 4);    // 需向堆区索要的空间

        void* new_allocated_area = ::malloc(bytes_to_get);
        allocated_area.push_back(new_allocated_area);   // 将已分配的内存加入allocated_area链表供mem_pool析构时归还分配过的内存

        begin_free = static_cast<char*>(new_allocated_area);
        if (begin_free == nullptr) {    // ::malloc失败，将返回nullptr
            list_header_t target_list = nullptr;
            obj* ptr = nullptr;

            // 从free_list中找到足以满足一块chunk的内存，并将其编入memory_pool
            for (size_t i = size; i <= kMax_Bytes; i += kALIGN) {
                target_list = free_lists + GET_FREELIST_INDEX(i);
                ptr = *target_list;
                if (ptr != nullptr) {
                    *target_list = ptr->next;
                    begin_free = static_cast<char*>(static_cast<void*>(ptr));
                    end_free = begin_free + i; 
                    return chunk_alloc(size, n_objs);
                }
            }

            end_free = nullptr; // memory_pool和free_list中都没有内存了
            // 向master_alloc请求分配内存，这会导致抛出std::bad_alloc或内存不足情况得以改善
            begin_free = static_cast<char*>(master_alloc::allocate(bytes_to_get));
        }
        heap_size += bytes_to_get;
        end_free = begin_free + bytes_to_get;
        return chunk_alloc(size, n_objs);
    }
}

#endif
