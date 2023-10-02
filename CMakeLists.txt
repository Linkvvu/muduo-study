cmake_minimum_required(VERSION 3.5)

project(muduo)

include_directories(${CMAKE_SOURCE_DIR})

set(
    MUDUO_NET
    EventLoop.cpp
    Poller.cpp
    Channel.cpp
    poller/PollPoller.cpp
    Timer.cpp
    TimerQueue.cpp
    logger.cpp
)

set(UNIT_TEST ON)

if(UNIT_TEST)
    add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif(UNIT_TEST)

add_library(muduoNet ${MUDUO_NET})
