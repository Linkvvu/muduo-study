link_libraries(muduo_net)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests)

add_executable(reactor_unittest1 reactor_unittest1.cpp)
add_executable(reactor_unittest2 reactor_unittest2.cpp)
add_executable(reactor_unittest3 reactor_unittest3.cpp)
add_executable(reactor_unittest4 reactor_unittest4.cpp)
add_executable(reactor_unittest5 reactor_unittest5.cpp)
add_executable(reactor_unittest6 reactor_unittest6.cpp)

if(BOOSTTEST_LIBRARY)
    add_executable(inetAddress_unittest inetAddress_unittest.cpp)
    target_link_libraries(inetAddress_unittest muduo_net muduo_net boost_unit_test_framework)
endif(BOOSTTEST_LIBRARY)
