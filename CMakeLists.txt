cmake_minimum_required(VERSION 3.5)

project(muduo)

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(
    MUDUO_NET
    EventLoop.cpp
    Poller.cpp
    Channel.cpp
    poller/PollPoller.cpp
    Timer.cpp
    TimerQueue.cpp
    logger.cpp
    Bridge.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp
    base/SocketOps.cpp
    InetAddr.cpp
    TcpConnection.cpp
    Socket.cpp
    Acceptor.cpp
    TcpServer.cpp
    base/MemPool.cpp
    Buffer.cpp
)

set(UNIT_TEST ON)

if(UNIT_TEST)
    add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif(UNIT_TEST)

add_library(muduoNet ${MUDUO_NET})
target_link_libraries(muduoNet pthread)
