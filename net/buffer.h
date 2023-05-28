/// This is a public header file, it must only include public header files.

#include <muduo/base/Types.h>
#include <muduo/net/endian.h>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <algorithm>

namespace muduo {
namespace net {

class buffer {
    using size_t = std::size_t;
public:
    static const std::size_t kCheapPrepend = 8;
    static const std::size_t KInitialSize = 1024;
    explicit buffer(const size_t initialSize = KInitialSize)
        : buf_(initialSize)
        , readIndex_(kCheapPrepend)
        , writeIndex_(kCheapPrepend) {}

    size_t readable_bytes() const
    { return writeIndex_ - readIndex_; }    

    size_t writeable_bytes() const
    { return buf_.size() - writeIndex_; }

    size_t prependable_bytes() const 
    { return readIndex_ - 0; }

    // char* beginWrite()
    // { return begin() + writeIndex_; }

    const char* beginWrite() const
    { return begin() + writeIndex_; }

    const char* peek() const
    { return begin() + readIndex_; }

    void swap(buffer& another) {
        buf_.swap(another.buf_);
        std::swap(readIndex_, another.readIndex_);
        std::swap(writeIndex_, another.writeIndex_);
    }

    const char* find_CRLF() const {
        const char* res = std::search(peek(), beginWrite(), KCRLF, KCRLF + 2);
        return res == beginWrite() ? nullptr : res;
    }

    const char* find_CRLF(const char* const start) const {
        assert(peek() <= start && start <= beginWrite());
        const char* res = std::search(start, beginWrite(), KCRLF, KCRLF + 2);
        return res == beginWrite() ? nullptr : res;
    }

    void retrieve_all() {
        readIndex_ = kCheapPrepend;
        writeIndex_ = kCheapPrepend;
    }

    void retrieve(size_t len) {
        assert(len <= readable_bytes());
        if (len < readable_bytes()) {
            readIndex_ += len;
        } else {
            retrieve_all();
        }
    }

    string retrieve_as_string(size_t len) {
        assert(len <= readable_bytes());
        string res(peek(), len);
        retrieve(len);
        return res;
    }
 
    string retrieve_all_as_string() {
        return retrieve_as_string(readable_bytes());
    }

    void retrieve_until(const char* const end) {
        assert(peek() <= end && end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieve_int64()
    { retrieve(sizeof(int64_t)); }

    void retrieve_int32()
    { retrieve(sizeof(int32_t)); }

    void retrieve_int16()
    { retrieve(sizeof(int16_t)); }

    void retrieve_int8()
    { retrieve(sizeof(int8_t)); }

    void ensure_writeable_bytes(const size_t len) {
        if (len > writeable_bytes()) {
            make_place(len);
        }
        assert(len <= writeable_bytes());
    }

    void append(const char* const data, const size_t len) {
        ensure_writeable_bytes(len);
        std::copy(data, data + len, beginWrite());
        has_writen(len);
    }

    void append(const void* const data, const size_t len) {
        append(static_cast<const char*>(data), len);
    }

    /// @brief Append int64_t using network endian 
    void append_int64(int64_t x) {
        int64_t be64 = sockets::hostToNetwork64(x);
        append(&be64, sizeof be64);
    }

    /// @brief Append int32_t using network endian 
    void append_int32(int32_t x) {
        int32_t be32 = sockets::hostToNetwork32(x);
        append(&be32, sizeof be32);
    }

    /// @brief Append int16_t using network endian 
    void append_int16(int16_t x) {
        int16_t be16 = sockets::hostToNetwork16(x);
        append(&be16, sizeof be16);
    }

    /// @brief Append int8_t using network endian 
    void append_int8(int8_t x) {
        append(&x, sizeof x);
    }

    int64_t peek_int64() const {
        assert(readable_bytes() >= sizeof(int64_t));
        int64_t be64 = 0;
        ::memcpy(&be64, peek(), sizeof be64);
        return sockets::networkToHost64(be64);
    }

    int64_t read_int64() {
        int64_t res = peek_int64();
        retrieve_int64();
        return res;
    }

    int32_t peek_int32() const {
        assert(readable_bytes() >= sizeof(int32_t));
        int32_t be32 = 0;
        ::memcpy(&be32, peek(), sizeof be32);
        return sockets::networkToHost32(be32);
    }

    int32_t read_int32() {
        int32_t res = peek_int32();
        retrieve_int32();
        return res;
    }

    int16_t peek_int16() const {
        assert(readable_bytes() >= sizeof(int16_t));
        int16_t be16 = 0;
        ::memcpy(&be16, peek(), sizeof be16);
        return sockets::networkToHost16(be16);
    }

    int16_t read_int16() {
        int16_t res = peek_int16();
        retrieve_int16();
        return res;
    }

    int8_t peek_int8() const {
        assert(readable_bytes() >= sizeof(int8_t));
        int8_t be8 = 0;
        ::memcpy(&be8, peek(), sizeof be8);
        return be8;
    }

    int8_t read_int8() {
        int8_t res = peek_int8();
        retrieve_int8();
        return res;
    }

    void prepend(const void* const data, size_t len) {
        assert(prependable_bytes() >= len);
        readIndex_ -= len;
        const char* tmp = static_cast<const char*>(data);
        std::copy(tmp, tmp + len, begin() + readIndex_);
    }

    /// @brief Prepend int64_t using network endian
    void prepend_int64(int64_t x) {
        int64_t be64 = sockets::hostToNetwork64(x);
        prepend(&be64, sizeof be64);
    }
    
    /// @brief Prepend int32_t using network endian
    void prepend_int32(int32_t x) {
        int32_t be32 = sockets::hostToNetwork32(x);
        prepend(&be32, sizeof be32);
    }

    /// @brief Prepend int16_t using network endian
    void prepend_int16(int16_t x) {
        int16_t be16 = sockets::hostToNetwork16(x);
        prepend(&be16, sizeof be16);
    }

    /// @brief Prepend int8_t using network endian
    void prepend_int8(int8_t x) {
        prepend(&x, sizeof x);
    }

    void shrink() { buf_.shrink_to_fit(); }

    size_t internal_capacity() const
    { return buf_.capacity(); }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t read_fd(const int fd, int* savedErrno);

private:
    const char* begin() const
    { return &(*buf_.begin()); }

    char* begin() { return &(*buf_.begin()); }

    void make_place(const size_t len);

    void has_writen(const size_t len)
    { writeIndex_ += len; }

    char* beginWrite()
    { return begin() + writeIndex_; }

private:
    std::vector<char> buf_;  // non-fixed buffer
    std::size_t readIndex_;
    std::size_t writeIndex_;

    static const char KCRLF[];
};

} // namespace net 
} // namespace muduo 