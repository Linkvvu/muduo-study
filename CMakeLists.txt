cmake_minimum_required(VERSION 3.0)

project(muduo-study)

add_compile_options("-Wall")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/base)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/net)