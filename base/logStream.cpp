#include <muduo/base/logStream.h>
#include <sstream>
#include <limits>
#include <iomanip>
#include <type_traits>
#include <cassert>

namespace muduo {
namespace detail {
        
template <int SIZE>
const char* fixed_buffer<SIZE>::debugString() {
    *cur_ = '\0';
    return data_;
}

// 显示实例化fixed_buffer类模板
template class fixed_buffer<KSmallBuffer>;
template class fixed_buffer<KLargeBuffer>;

} // namespace detail 

void logStream::static_check() {
    // std::numeric_limits<T>::digits10返回一个 T 类型的变量可以表示的最大十进制数字的位数
    // 此处的十进制数字的位数不包含符号位，小数点
    // (2^31)-1用十进制表示[2147483647]一共是十位数字，但其将最开头的2看作符号位，故std::numeric_limits<int>::digits10返回9
    // 因此，+1使得能存储[2147483647]这个数，再+1使得能存储负号，最后+1使得存储字符数组的终止符'\0'
    // 故std::numeric_limits<int>::digits10+3是字符数组需要满足的最小空间，否则会导致字符串截断

    static_assert(kMaxNumericSize  >= std::numeric_limits<double>::digits10+4); // 此处+4是给小数点分配空间
    static_assert(kMaxNumericSize  >= std::numeric_limits<long double>::digits10+4);
    static_assert(kMaxNumericSize  >= std::numeric_limits<long>::digits10+3);
    static_assert(kMaxNumericSize  >= std::numeric_limits<long long>::digits10+3);
    static_assert(kMaxNumericSize  >= std::numeric_limits<int>::digits10+3);
}

#if nullptr // muduo库的实现
const char digits[] = "9876543210123456789";
const char* zero = digits + 9;
static_assert(sizeof(digits) == 20, "failed");

template <typename T>
std::size_t convert(char destination[], T value) {
    T v = value;
    char* p = destination;
    do {
        int latest = static_cast<int>(v % 10);
        v /= 10;
        *p++ = zero[latest];
    } while (v != 0);
    if (value < 0) {
        *p++ = '-';
    }
    *p = '\0';
    std::reverse(destination, p);
    return p-destination;
}
#endif

logStream::self& logStream::operator<<(short v) {
    *this << static_cast<int>(v);
    return *this;
}

logStream::self& logStream::operator<<(unsigned short v) {
    *this << static_cast<unsigned int>(v);
    return *this;
}

logStream::self& logStream::operator<<(int v) {
    *this << std::to_string(v);
    return *this;
}

logStream::self& logStream::operator<<(unsigned int v) {
    *this << std::to_string(v);
    return *this;
}

logStream::self& logStream::operator<<(long v) {
    *this << std::to_string(v);
    return *this;
}

logStream::self& logStream::operator<<(unsigned long v) {
    *this << std::to_string(v);
    return *this;
}

logStream::self& logStream::operator<<(long long v) {
    *this << std::to_string(v);
    return *this;
}

logStream::self& logStream::operator<<(unsigned long long v) {
    *this << std::to_string(v);
    return *this;
}

#if nullptr
logStream::self& logStream::operator<<(double v) {
    std::ostringstream tmp;
    // std::streamsize ss = tmp.precision; // 保存原始的precision
    tmp << std::fixed << std::setprecision(2) << v;
    // no need call "std::unsetf(ios::fixed)" and "std::setprecision(ss)" to recover mode
    // 因为这些都是对临时的 ostringstream(tmp) 流对象的设置，而不是std::cout(标准输出流对象)
    *this << tmp.str();
    return *this;
}
#else

logStream::self& logStream::operator<<(double v) {
    if (buf_.free_size() >= kMaxNumericSize) {
        // "%.*g": std::numeric_limits<double>::digits10:(digits10返回值为double类型10进制最大位数，不包含符号位和小数点)替换*，保证浮点数完整输出
        int len = snprintf(buf_.current(), kMaxNumericSize, "%.*g", std::numeric_limits<double>::digits10, v);
        buf_.add(len);
    }
    return *this;
}
#endif

logStream::self& logStream::operator<<(const void* ptr) {
    // GCC中:
    //       32位的系统下 typedef "unsigned int" uintptr_t ; 
    //       64位的系统下 typedef "unsigned long" uintptr_t ; 
    uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
    std::ostringstream tmp;
    tmp << "0x" << std::hex << v;
    *this << tmp.str();
    return *this;
}

template <typename T>
Fmt::Fmt(const char* format, T val) {
    static_assert(std::is_arithmetic<T>::value == true, "type must be arithmetic");
    len_ = snprintf(buf_, sizeof buf_, format, val);
    assert(static_cast<std::size_t>(len_) <= sizeof buf_);
}

// Explicit instantiations
template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);

} // namespace muduo 