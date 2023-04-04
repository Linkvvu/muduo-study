set(MUDUO_BASE_SRCS
	timeStamp.cpp
	Exception.cpp
	Thread.cpp
)

add_library(muduo STATIC ${MUDUO_BASE_SRCS})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
