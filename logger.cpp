#include <logger.h>

thread_local char muduo::tl_errnoBuffer[1024];

const char* muduo::strerror_thread_safe(int errnum) {
    // the GNU version is used by default
    return strerror_r(errnum, tl_errnoBuffer, 1024);
}