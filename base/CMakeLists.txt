set(MUDUO_BASE_SRCS
	timeStamp.cpp
	Exception.cpp
	Thread.cpp
	countDownLatch.cpp
	threadpool.cpp
)

add_library(muduo STATIC ${MUDUO_BASE_SRCS})
target_link_libraries(muduo pthread)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
