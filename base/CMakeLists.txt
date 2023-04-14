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

find_package(Boost REQUIRED)
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)

add_library(muduo STATIC ${MUDUO_BASE_SRCS})
target_link_libraries(muduo pthread)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
