cmake_minimum_required(VERSION 3.5)

project(muduo)

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall")

set(
    MUDUO_NET
    EventLoop.cpp
    Poller.cpp
    Channel.cpp
    poller/PollPoller.cpp
    Timer.cpp
    TimerQueue.cpp
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
    base/LogStream.cpp
    base/Logging.cpp
    base/LogFile.cpp
    base/AsyncLogging.cpp
)

set(UNIT_TEST ON)

if(UNIT_TEST)
    add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif(UNIT_TEST)

add_library(muduoNet ${MUDUO_NET})
target_link_libraries(muduoNet pthread)
