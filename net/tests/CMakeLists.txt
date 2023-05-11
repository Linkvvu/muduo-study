link_libraries(muduo_net)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests)

add_executable(reactor_unittest1 reactor_unittest1.cpp)
add_executable(reactor_unittest2 reactor_unittest2.cpp)
add_executable(reactor_unittest3 reactor_unittest3.cpp)
add_executable(reactor_unittest4 reactor_unittest4.cpp)