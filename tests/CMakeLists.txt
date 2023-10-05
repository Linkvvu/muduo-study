add_executable(thread_local_unittest thread_local.cc)
target_link_libraries(thread_local_unittest pthread)

find_package(GTest REQUIRED)
add_executable(Channel_unittest Channel_unittest.cc)
target_link_libraries(Channel_unittest muduoNet GTest::gtest_main GTest::gtest)

find_package(fmt REQUIRED)
add_executable(TimerQueue_unittest TimerQueue_unittest.cc)
target_link_libraries(TimerQueue_unittest "muduoNet" "fmt" "GTest::gtest_main" "GTest::gtest")

add_executable(Bridge_unittest Bridge_unittest.cc)
target_link_libraries(Bridge_unittest "pthread" "muduoNet" "fmt")

add_executable(EventLoopThread_unittest EventLoopThread_unittest.cc)
target_link_libraries(EventLoopThread_unittest "pthread" "muduoNet" "fmt")

add_executable(EventLoopThreadPool_unittest EventLoopThreadPool_unittest.cc)
target_link_libraries(EventLoopThreadPool_unittest "pthread" "muduoNet" "fmt" "GTest::gtest" "GTest::gmock_main")