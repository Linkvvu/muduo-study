#if !defined(MUDUO_BASE_LOG_STREAM_H)
#define MUDUO_BASE_LOG_STREAM_H
#include <string>
#include <cassert>
#include <cstdint>
#include <cstring>

namespace muduo {
namespace base {

namespace detail {

template <int SIZE>
class FixedBuffer {
    using size_t = std::size_t;
    
public:
    FixedBuffer()
        : cur_(data_)
        { }
    
    FixedBuffer(const FixedBuffer& buf) 
        : FixedBuffer()
    {
        std::memcpy(data_, buf.data_, buf.GetLength());
        cur_ += buf.GetLength();
    }

    FixedBuffer(FixedBuffer&& buf) {
        data_ = buf.data_;
        data_ = buf.cur_;

        buf.data_ = nullptr;
        buf.cur_ = nullptr;
    }

    const char* Data() const
    { return data_; }

    void Append(const void* data, size_t len) {
        if (Avail() > len) {
            std::memcpy(Current(), data, len);
            AddLength(len);
        }
    }

    const char* Current() const 
    { return cur_; }

    size_t GetLength() const
    { return cur_ - data_; }

    size_t Avail() const
    { return End() - cur_; } 

    void Reset() 
    {
        std::memset(data_, 0, sizeof data_);
        cur_ = data_;
    }

    std::string ToString() const 
    { return std::string(data_, GetLength()); }

private:
    char* Current()
    { return cur_; }

    const char* End() const
    { return data_ + sizeof data_; }

    void AddLength(size_t len)
    {
        // assert(len < Avail());
        cur_ += len;
    }

private:
    char data_[SIZE] {0};
    char* cur_;
};

} // namespace detail 

class LogStream {
    using self = LogStream;

    /* non-copyable & non-moveable */
    LogStream(const LogStream&) = delete;
    LogStream& operator=(const LogStream&) = delete;

public:
    static const size_t kLargeBuffer = 1024*1024; 
    static const size_t kSmallBuffer = 1024;

#ifdef MUDUO_LOG_LARGE_BUFFER
    using Buffer = detail::FixedBuffer<kLargeBuffer>;
#else
    using Buffer = detail::FixedBuffer<kSmallBuffer>;
#endif

    LogStream()
        : buf_(Buffer())
        { }

    void Append(const void* data, size_t len) 
    { buf_.Append(data, len); }

    const Buffer& GetInternalBuf() const
    { return buf_; }

    void ResetBuffer()
    { buf_.Reset(); }

    template <typename T>
    self& operator<<(T val) {
        std::string formated = std::to_string(val);
        return *this << formated; 
    }

    /* Member functions take precedence over template functions */
    self& operator<<(const std::string& str) {
        Append(str.c_str(), str.size());
        return *this;
    }

    self& operator<<(char c) {
        Append(&c, sizeof c);
        return *this;
    }

    self& operator<<(const void* ptr);

    self& operator<<(void* ptr)
    { return *this << const_cast<const void*>(ptr); }

    self& operator<<(const char* str);

    self& operator<<(char* str)
    { return *this << const_cast<const char*>(str); }

private:
    Buffer buf_;
};

/* Must be here, 必须先特例化 'const char*' , 避免特例化在实例化之后 */


template<>
LogStream::self& LogStream::operator<< <bool>(bool boolean);

template<>
LogStream::self& LogStream::operator<< <double>(double d);

template<>
LogStream::self& LogStream::operator<< <float>(float f);

} // namespace base 
} // namespace muduo 

#endif // MUDUO_BASE_LOG_STREAM_H
