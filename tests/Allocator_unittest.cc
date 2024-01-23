#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <muduo/EventLoop.h>
#include <vector>
#include <memory>
#include <iostream>

#define NUM 1000

using namespace muduo;

int main() {
    EventLoop loop;
    auto pool = std::make_shared<base::MemoryPool>(&loop);
    base::allocator<int> alloc = base::allocator<int>(pool.get());
    auto vec = new std::vector<int, muduo::base::allocator<int>>(alloc);
    
    for (int i = 0; i < NUM; i++) {
        vec->push_back(i);
    }

    for (int i = 0; i < NUM; i++) {
        // std::cout << vec->at(i) << std::endl;
    }

    delete vec;
}
