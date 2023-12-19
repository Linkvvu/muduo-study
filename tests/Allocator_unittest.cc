#include <muduo/base/allocator/sgi_stl_construct.h>
#include <muduo/base/allocator/sgi_stl_alloc.h>
#include <muduo/base/allocator/mem_pool.h>
#include <vector>
#include <iostream>

#define NUM 1000

using namespace muduo::base;

int main() {
    muduo::base::MomoryPool pool;
    auto vec = new std::vector<int, muduo::base::alloctor<int>>(muduo::base::alloctor<int>(&pool));
    
    for (int i = 0; i < NUM; i++) {
        vec->push_back(i);
    }

    for (int i = 0; i < NUM; i++) {
        // std::cout << vec->at(i) << std::endl;
    }

    delete vec;
}