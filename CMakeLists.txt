cmake_minimum_required(VERSION 3.5)

project(muduo)

include_directories(${CMAKE_SOURCE_DIR})

set(
    MUDUO_NET
    EventLoop.cpp
)

if(UNIT_TEST)
    add_subdirectory(${CMAKE_SOURCE_DIR}/unitTest)
endif(UNIT_TEST)


add_library(muduoNet ${MUDUO_NET})
