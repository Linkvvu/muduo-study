project(shudu-resolver)

link_libraries(muduo_net)

set(
    SRCS
    main.cpp
    resolver.cpp
    shudu-algorithm/shuduku.cpp
)

add_executable(${PROJECT_NAME} ${SRCS})