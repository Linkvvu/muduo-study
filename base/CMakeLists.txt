set(MUDUO_BASE_SRCS
	timeStamp.cpp
	Exception.cpp
	Thread.cpp
	countDownLatch.cpp
	threadpool.cpp
	logStream.cpp
	logFile.cpp
	logging.cpp
)

add_library(muduo_base STATIC ${MUDUO_BASE_SRCS})
target_link_libraries(muduo_base pthread)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
