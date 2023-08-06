project(chargen)

link_libraries(muduo_net)

add_executable("${PROJECT_NAME}_server" main.cpp chargen.cpp)
add_executable("${PROJECT_NAME}_client" client.cpp)