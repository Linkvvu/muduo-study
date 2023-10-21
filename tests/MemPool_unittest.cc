#include <base/MemPool.h>
#include <memory>
#include <iostream>

int main() {
    using namespace muduo::base;
    MemPool* p = MemPool::CreateMemoryPool(MEM_POOL_MIN_SIZE);
    // std::unique_ptr<MemPool, decltype([](MemPool*){})> up;   // since C++20
    std::unique_ptr<MemPool, void(*)(MemPool*)> up(p, &MemPool::DestroyMemoryPool);
    for (int i = 10; i < 5000; i++) {
        void* t = up->Palloc(i);
        p->Pfree(t);
    }
    std::cout << "---test---" << std::endl;
}