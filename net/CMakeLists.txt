set(MUDUO_NET_SRCS
    eventLoop.cpp
    poller.cpp
    channel.cpp
    poller/pollPoller.cpp
    poller/epollPoller.cpp
    poller/defaultPoller.cpp
    timer.cpp
    timerQueue.cpp
    eventLoopThread.cpp
)

add_library(muduo_net STATIC ${MUDUO_NET_SRCS})
target_link_libraries(muduo_net muduo_base)

install(TARGETS muduo_net DESTINATION lib)
set(HEADERS
    channel.h
    eventLoop.h
    timerId.h
    eventLoopThread.h
)
install(FILES ${HEADERS} DESTINATION include/muduo/net)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)