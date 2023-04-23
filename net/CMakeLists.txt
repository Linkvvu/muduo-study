set(MUDUO_NET_SRCS
    eventLoop.cpp
)

add_library(muduo_net STATIC ${MUDUO_NET_SRCS})
target_link_libraries(muduo_net muduo_base)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)