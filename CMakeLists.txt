cmake_minimum_required(VERSION 3.5)

project(muduo)

string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)

if(NOT BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  set(BUILD_TYPE "RELEASE")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
elseif(BUILD_TYPE STREQUAL "DEBUG")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
endif()

message(STATUS BUILD_TYPE=${BUILD_TYPE})

set(CMAKE_CXX_STANDARD "17")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall")
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${PROJECT_BINARY_DIR}/lib")

find_package(Boost REQUIRED)
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

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

message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})

if(MUDUO_UNIT_TESTS STREQUAL "ON" OR
      MUDUO_UNIT_TESTS STREQUAL "On" OR
      MUDUO_UNIT_TESTS STREQUAL "on" OR
      MUDUO_UNIT_TESTS STREQUAL "yes" OR
      MUDUO_UNIT_TESTS STREQUAL "y")
  add_subdirectory(${CMAKE_SOURCE_DIR}/tests)
endif()

add_library(muduoNet ${MUDUO_NET})
target_link_libraries(muduoNet pthread)

set(
  HEADERS
  Buffer.h
  Callbacks.h
  Channel.h
  EventLoop.h
  EventLoopThread.h
  EventLoopThreadPool.h
  InetAddr.h
  TcpConnection.h
  TcpServer.h
  TimerType.h
)

set(
  BASE_HEADERS
  base/AsyncLogging.h
  base/Endian.h
  base/LogFile.h
  base/Logging.h
  base/LogStream.h
)

install(FILES ${HEADERS} DESTINATION include/muduo)
install(FILES ${BASE_HEADERS} DESTINATION include/muduo/base)
install(TARGETS muduoNet DESTINATION lib)