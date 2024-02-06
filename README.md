# Overview
The repository is for Learning high-performance network library (on Linux) based on Reactor pattern
> Muduo is a multithreaded C++ network library based on
the Reactor pattern.
# Reference
[github.com/chenshuo/muduo](https://github.com/chenshuo/muduo)
# Quick Start
## Requires
* cpp-17 least
* Boost.Endian (A header-only lib for Endian-conversion)
* GTest (if need to compile unit tests)
## Compile
``` bash
# If need to compile test files, 
# define environment variable "MUDUO_UNIT_TESTS=ON"
# to enable memory-pool
# define environment variable "MUDUO_USE_MEMPOOL=ON"

# See build.sh for details
bash build.sh
```
# Introduction
* 基于 **"事件驱动"** 的Reactor网络编程模型
* 支持多种Reactor模式
    * Single Reactor - 单线程模式
    * Multithreaded Reactor - 主从Reactor模式
    * Multithreaded Reactor + threadPool
* 采用「one loop per thread」线程模型 + non-blocking IO
* 基于双缓冲区机制实现**异步日志**
* 基于**优先队列**实现**定时器**管理结构
* 遵行"RAII"思想，使用智能指针管理内存
* 参考"SGI STL-allocator"实现了**循环级内存池**
* 支持select\\poll\\epoll 3种 IO-multiplexing

# 并发模型
### Single Reactor
![Single Reactor](https://gh-images.oss-cn-beijing.aliyuncs.com/single-reactor.png)
### Multithreaded Reactor + threadPool
![Multithreaded Reactor](https://gh-images.oss-cn-beijing.aliyuncs.com/multiple-reactor.png)
