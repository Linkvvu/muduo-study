cmake_minimum_required(VERSION 3.0)

project(muduo-study)

set(CMAKE_BUILD_TYPE "Debug")
set(UNIX_API_TEST on)

add_compile_options("-Wall")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Boost REQUIRED)
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)
message(STATUS "boost_unittest_framework PATH: ${BOOSTTEST_LIBRARY}")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/base)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/net)

if(UNIX_API_TEST)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/UNIX-API_examples)
endif(UNIX_API_TEST)
