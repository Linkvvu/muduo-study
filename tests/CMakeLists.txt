find_package(GTest REQUIRED)

message(STATUS "Will compile unit-tests.")

add_executable(thread_local_unittest thread_local.cc)
target_link_libraries(thread_local_unittest pthread)

add_executable(TimerQueue_unittest TimerQueue_unittest.cc)
target_link_libraries(TimerQueue_unittest "muduoNet")

add_executable(Bridge_unittest Bridge_unittest.cc)
target_link_libraries(Bridge_unittest "pthread" "muduoNet")

add_executable(EventLoopThread_unittest EventLoopThread_unittest.cc)
target_link_libraries(EventLoopThread_unittest "pthread" "muduoNet")

add_executable(EventLoopThreadPool_unittest EventLoopThreadPool_unittest.cc)
target_link_libraries(EventLoopThreadPool_unittest "muduoNet" "GTest::gtest" "GTest::gmock_main")

add_executable(Endian_unittest Endian_unittest.cc)
target_link_libraries(Endian_unittest "GTest::gtest" "GTest::gtest_main")

add_executable(Sockaddr_unittest Sockaddr_unittest.cc)
target_link_libraries(Sockaddr_unittest "GTest::gtest" "GTest::gtest_main")

add_executable(Reactor_unittest01 Reactor_unittest01.cc)
target_link_libraries(Reactor_unittest01 muduoNet)

add_executable(MemPool_unittest MemPool_unittest.cc)
target_link_libraries(MemPool_unittest muduoNet "GTest::gtest" "GTest::gtest_main")

add_executable(Buffer_unittest Buffer_unittest.cc)
target_link_libraries(Buffer_unittest muduoNet "GTest::gtest" "GTest::gtest_main")

add_executable(TcpConnection_IO_unittest TcpConnection_IO_unittest.cc)
target_link_libraries(TcpConnection_IO_unittest muduoNet)

add_executable(Reactor_unittest02 Reactor_unittest02.cc)
target_link_libraries(Reactor_unittest02 muduoNet)

add_executable(Logging_unittest Logging_unittest.cc)
target_link_libraries(Logging_unittest muduoNet "GTest::gtest" "GTest::gtest_main")

add_executable(LogFile_unittest LogFile_unittest.cc)
target_link_libraries(LogFile_unittest muduoNet)

add_executable(AsyncLogging_unittest AsyncLogging_unittest.cc)
target_link_libraries(AsyncLogging_unittest muduoNet)