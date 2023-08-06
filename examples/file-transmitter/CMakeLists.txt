project(file-transmitter)

link_libraries(muduo_net)

add_executable(${PROJECT_NAME} transmitter.cpp main.cpp)

add_executable(${PROJECT_NAME}_RAII transmitter-RAII.cpp)