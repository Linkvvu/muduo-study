#if !defined(MUDUO_LOGGER_H)
#define MUDUO_LOGGER_H

#include <cstring>

namespace muduo {

    extern thread_local char tl_errnoBuffer[1024]; // initial state is NULL
    const char* strerror_thread_safe(int errnum);

} // namespace muduo 

#endif // MUDUO_LOGGER_H



    


