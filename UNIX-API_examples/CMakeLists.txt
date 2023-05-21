set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_executable(timerfd_test01 timerfd_test01.cpp)
add_executable(timerfd_test02 timerfd_test02.cpp)
add_executable(eventfd_test eventfd_test.cpp)
add_executable(SO_ERROR option_SO_ERROR.cpp)