# Overview
The repository is for Learning high-performance network library (on Linux) based on Reactor pattern
> Muduo is a multithreaded C++ network library based on
the Reactor pattern.
# Reference
[github.com/chenshuo/muduo](https://github.com/chenshuo/muduo)
# Quick Start
## Requires
* cpp-17 (for std::any only)
* GTest (if need to compile unit tests)
## Compile
``` bash
mkdir build && cd build
cmake .. (-DUNIT_TEST=ON)
make && make install
```
# Introduction
* event-driven mode
* multithreaded-Reactor + threadPool(optional) mode
* one EventLoop per thread & non-blocking IO
* based on callback
* Asynchronous logger
# TODO(optimize):
- [x] 使用cpp11-17新特性
- [x] 使用priority-queue重构定时器队列 
- [x] 使用RAII, 避免显式的new/delete
- [x] 封装*eventfd(2)* class，尽可能实现高内聚、低耦合
- [x] 新增连接级内存池