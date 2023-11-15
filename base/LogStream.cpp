#include <muduo/base/LogStream.h>
#include <sstream>  // std::stringstream
#include <iomanip>  // std::setprecision

using namespace muduo;
using namespace base;

template<>
LogStream::self& LogStream::operator<< <char*>(char* str) {
    return *this << const_cast<const char*>(str);
}

template<>
LogStream::self& LogStream::operator<< <bool>(bool boolean) {
    if (boolean) {
        return *this << "true";
    } else {
        return *this << "false";
    }
}

template<>
LogStream::self& LogStream::operator<< <double>(double d){
    std::ostringstream out_str_stream;
    out_str_stream << std::setprecision(12) << d;
    return *this << out_str_stream.str();
}

template<>
LogStream::self& LogStream::operator<< <float>(float f) {
    return *this << static_cast<double>(f);
}

LogStream::self& LogStream::operator<<(const void* ptr) {
    if (ptr) {
        std::ostringstream out_str_stream;
        out_str_stream << ptr;
        return *this << out_str_stream.str();
    } else {
        return *this << "0x0";
    }
}

LogStream::self& LogStream::operator<<(const char* str) {
    if (str) {
        Append(static_cast<const void*>(str), std::strlen(str));
    } else {
        Append("(null)", 6);
    }
    return *this;
}
