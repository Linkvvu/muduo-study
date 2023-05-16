#if !defined(MUDUO_BASE_LOGSTREAM_H)
#define MUDUO_BASE_LOGSTREAM_H

#include <boost/noncopyable.hpp>
#include <sstream>
#include <cstring>

namespace muduo {
namespace detail {
    
const std::size_t KSmallBuffer = 4000;
const std::size_t KLargeBuffer = 4000 * 1000;

template <int SIZE> // 由于数组大小在编译期就要确定，故使用"非类型模板参数"
class fixed_buffer : boost::noncopyable {
public:
    fixed_buffer() : cur_(data_) {}
    ~fixed_buffer() = default;
    
    void append(const char* buf, std::size_t len) {
        if (static_cast<std::size_t>(free_size()) > len) {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }

    const char* data() const { return data_; }

    int size() const { return static_cast<int>(cur_ - data_); }

    char* current() { return cur_; }

    int free_size() const { return static_cast<int>((end() - cur_)); }

    void add(std::size_t len) { cur_ += len; }

    void reset() { cur_ = data_; }

    void bzero() { ::bzero(data_, sizeof data_); }

    // for used by GDB
    const char* debugString();

    // for used by unit-test
    std::string to_string_t() const { return std::string(data_, size()); }

private:
    const char* end() const { return data_ + sizeof(data_); }

private:
    char data_[SIZE];
    char* cur_;
};

} // namespace detail 

class logStream : private boost::noncopyable {
    static const int kMaxNumericSize = 22; // 算数类型(int\long\double)转换为字符串后(十进制)的最大位数
    using self = logStream;
public:
    // using buffer_t = detail::fixed_buffer<detail::KSmallBuffer>;
    typedef detail::fixed_buffer<detail::KSmallBuffer> buffer_t;
    
    self& operator<<(bool v) {
        buf_.append(v ? "1" : "0", 1);
        return *this;
    }

    self& operator<<(char v) {
        buf_.append(&v, 1);
        return *this;
    }

    self& operator<<(const std::string& v) {
        buf_.append(v.c_str(), v.size());
        return *this;
    }

    self& operator<<(const char* v) {
        if (v != nullptr)
            buf_.append(v, std::strlen(v));
        return *this;
    }

    // self& operator<<(const StringPiece& v) {
    //     buffer_.append(v.data(), v.size());
    //     return *this;
    // }

    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);

    self& operator<<(const void*);

    self& operator<<(double);
    self& operator<<(float v) {
        *this << static_cast<double>(v);
        return *this;
    }

    void append(const char* data, std::size_t len) { buf_.append(data, len); }
    const buffer_t& buffer() const { return buf_; }
    void reset_buffer() { buf_.reset(); }

private:
    void static_check();

private:
    buffer_t buf_;
};

class Fmt : boost::noncopyable {
public:
    template <typename T>
    Fmt(const char* format, T val);

    const char* data() const { return buf_; }
    const std::size_t size() const { return static_cast<std::size_t>(len_); }

private:
    char buf_[32];
    int len_;
};

inline logStream& operator<<(logStream& ls, const Fmt& fmt) {
    ls.append(fmt.data(), fmt.size());
    return ls;
}

} // namespace muduo 

#endif // MUDUO_BASE_LOGSTREAM_H
