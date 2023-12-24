#include <muduo/base/allocator/sgi_stl_construct.h>
#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <muduo/base/allocator/mem_pool.h>
#include <vector>
#include <iostream>

#define NUM 1000

using namespace muduo;

int main() {
    base::alloctor<int> alloc = base::alloctor<int>(std::make_shared<base::MemoryPool>());
    auto vec = new std::vector<int, muduo::base::alloctor<int>>(alloc);
    
    for (int i = 0; i < NUM; i++) {
        vec->push_back(i);
    }

    for (int i = 0; i < NUM; i++) {
        // std::cout << vec->at(i) << std::endl;
    }

    delete vec;
}