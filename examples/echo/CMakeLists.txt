project(echo)

link_libraries(muduo_net)

add_executable(${PROJECT_NAME}_server server.cpp)
add_executable(${PROJECT_NAME}_client client.cpp)