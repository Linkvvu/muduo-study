#if !defined(MUDUO_BUFFER_H)
#define MUDUO_BUFFER_H
#include <vector>
#include <cassert>
#include <string>
namespace muduo {

/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

/// a copyable buffer for send/recv datas to/from file descriptor
class Buffer {
public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

public:
    Buffer(const Buffer&) = default;
    Buffer& operator=(const Buffer&) = default;
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize, 0)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {
        assert(ReadableBytes() == 0);
        assert(WriteableBytes() == initialSize);
        assert(PrependableBytes() == kCheapPrepend);
    }

    size_t ReadableBytes() const
    { return writerIndex_ - readerIndex_; }

    size_t WriteableBytes() const
    { return buffer_.size() - writerIndex_; }

    size_t PrependableBytes() const
    { return readerIndex_; }

    const char* Peek() const
    { return Begin() + readerIndex_; }

    // Retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    void Retrieve(size_t len) {
        assert(len <= ReadableBytes());
        if (len < ReadableBytes()) {
            readerIndex_ += len;
        } else {
            RetrieveAll();
        }
    }

    void RetrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string RetrieveAsString(size_t len) {
        assert(len <= ReadableBytes());
        std::string result(Peek(), len);
        Retrieve(len);
        return result;
    }

    std::string RetrieveAllAsString() 
    { return RetrieveAsString(ReadableBytes()); }

    const char* BeginWrite() const
    { return Begin() + writerIndex_; }

    void HasWritten(size_t len) {
        assert(len <= WriteableBytes());
        writerIndex_ += len;
    }

    void Append(const char* data, size_t len) {
        EnsureWriteableBytes(len);
        std::copy(data, data+len, BeginWrite());
        HasWritten(len);
    }

    void Append(const std::string& s)
    { Append(s.c_str(), s.size()); }

    size_t InternalCapacity() const
    {
        return buffer_.capacity();
    }

    void Prepend(const void* data, size_t len) {
        assert(len <= PrependableBytes());
        readerIndex_ -= len;
        const char* d = static_cast<const char*>(data);
        std::copy(d, d+len, Begin() + readerIndex_);
    }

    void Shrink()
    { buffer_.shrink_to_fit(); }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t ReadFd(int fd, int* savedErrno);

private:
    const char* Begin() const
    { return &buffer_.front(); }

    char* Begin()
    { return &buffer_.front(); }

    char* BeginWrite()
    { return Begin() + writerIndex_; }

    void EnsureWriteableBytes(size_t len) {
        if (WriteableBytes() < len) {
            BroadenSpace(len);
        }
        assert(len <= WriteableBytes());
    }

    void BroadenSpace(size_t len) {
        if (WriteableBytes() + PrependableBytes() < len + kCheapPrepend) {
            buffer_.resize(writerIndex_+len);
        } else {
            // move readable data to the front, make space inside buffer
            assert(kCheapPrepend < readerIndex_);
            size_t readable = ReadableBytes();
            std::copy(Begin()+readerIndex_,
                        Begin()+writerIndex_,
                        Begin()+kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert(readable == ReadableBytes());
        }
    }

private: 
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

} // namespace muduo 

#endif // MUDUO_BUFFER_H
