cmake_minimum_required(VERSION 3.5)

project(muduo CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Current BUILD_TYPE=${CMAKE_BUILD_TYPE}")

if(CMAKE_PROJECT_NAME STREQUAL "muduo")
  option(MUDUO_USE_MEMPOOL "Enable memory pool" ON)
  option(MUDUO_UNIT_TESTS "Build unit-tests of muduo" OFF)
endif()

if(MUDUO_USE_MEMPOOL)
  message(STATUS "Will compile memory-pool module")
endif(MUDUO_USE_MEMPOOL)


set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-Wall")

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Define NDEBUG when compile whith g++
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Set graceful project layout
include(GNUInstallDirs)

configure_file(config.h.in ${CMAKE_INSTALL_INCLUDEDIR}/muduo/config.h)

# Set the output path for binary for build-tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
# Set the output path for static libary for build-tree
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
# Set the output path for dynamic libary for build-tree
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})

# print current compile flags
string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UPPER)
string(REPLACE ";" " " CXX_FLAGS_LIST "${CMAKE_CXX_FLAGS}")
string(REPLACE ";" " " CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}_LIST "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}}")
message(STATUS "CXX_FLAGS = " ${CXX_FLAGS_LIST} " " ${CXX_FLAGS_${CMAKE_BUILD_TYPE_UPPER}_LIST})

# include required directory of header
include_directories(${CMAKE_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR})
include_directories(${Boost_INCLUDE_DIRS})

# collect sources
set(
    MUDUO_NET_SOURCES
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
    Buffer.cpp
    Connector.cpp
    TcpClient.cpp
    base/LogStream.cpp
    base/Logging.cpp
    base/LogFile.cpp
    base/AsyncLogging.cpp
    base/ThreadPool.cpp
    base/allocator/mem_pool.cpp
)

# make library
add_library(muduoNet STATIC ${MUDUO_NET_SOURCES})
set_target_properties( 
  muduoNet
    PROPERTIES 
    OUTPUT_NAME muduo
)

target_link_libraries(muduoNet PUBLIC pthread)

# compile unit-tests
if(MUDUO_UNIT_TESTS)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
  message(STATUS "Will compile unit-tests.")
endif()

# install
set(
  PUB_HEADERS
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
  TcpClient.h
  ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/muduo/config.h
)

set(
  PUB_BASE_HEADERS 
  base/AsyncLogging.h
  base/Endian.h
  base/LogFile.h
  base/Logging.h
  base/LogStream.h
  base/ThreadPool.h
)

set(
  PUB_BASE_ALLOCATOR_HEADERS 
  base/allocator/mem_pool.h
  base/allocator/sgi_stl_alloc.h
  base/allocator/Allocatable.h
)

install(FILES ${PUB_HEADERS} DESTINATION include/muduo)
install(FILES ${PUB_BASE_HEADERS} DESTINATION include/muduo/base)
install(FILES ${PUB_BASE_ALLOCATOR_HEADERS} DESTINATION include/muduo/base/allocator)
install(TARGETS muduoNet) # DESTINATION ${CMAKE_INSTALL_LIBDIR}
