add_executable(thread_local_unittest thread_local.cc)
target_link_libraries(thread_local_unittest pthread)

find_package(GTest REQUIRED)
add_executable(EventLoop_unittest01 EventLoop_unittest01.cc)
target_link_libraries(EventLoop_unittest01 muduoNet GTest::gtest_main GTest::gtest)

find_package(fmt REQUIRED)
add_executable(EventLoop_unittest02 EventLoop_unittest02.cc)
target_link_libraries(EventLoop_unittest02 "muduoNet" "fmt" "GTest::gtest_main" "GTest::gtest")