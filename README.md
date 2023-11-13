# Overview
The repository is for Learning high-performance network library (on Linux) based on Reactor pattern
> Muduo is a multithreaded C++ network library based on
the Reactor pattern.
# Reference
[github.com/chenshuo/muduo](https://github.com/chenshuo/muduo)
# Quick Start
## Requires
* cpp-17 (for std::any only)
* Boost (for Endian-conversion only)
* GTest (if need to compile unit tests)
## Compile
``` bash
# If need to compile test files, 
# define environment variable "MUDUO_UNIT_TESTS=ON"

# See build.sh for details
bash build.sh
```
# Introduction
* event-driven mode
* multithreaded-Reactor + threadPool(optional) mode
* one EventLoop per thread & non-blocking IO
* base on callback
* support asynchronous logger
# TODO(optimize):
- [x] 使用priority-queue重构定时器队列 
- [x] 项目遵从"RAII"思想, 避免显式的new/delete
- [x] 新增连接级内存池
- [x] 尽可能地使用C++11-17特性
- [x] 尽可能地实现高内聚、低耦合