#include <muduo/net/buffer.h>
#include <muduo/net/socketsOPs.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

// define data member
const char buffer::KCRLF[] = "\r\n";
const size_t buffer::kCheapPrepend;
const size_t buffer::KInitialSize;

void buffer::make_place(const size_t len) {
    if (prependable_bytes() + writeable_bytes() < len + kCheapPrepend) {
        buf_.resize(writeIndex_+len);
    } else {
        assert(readIndex_ > kCheapPrepend);
        size_t readable = readable_bytes();
        std::move(begin()+readIndex_, begin()+writeIndex_, begin()+kCheapPrepend);
        readIndex_ = kCheapPrepend;
        writeIndex_ = kCheapPrepend + readable;
        assert(readable == readable_bytes());
    }
}

ssize_t buffer::read_fd(const int fd, int* savedErrno) {
    // saved an ioctl()/FIONREAD call to tell how much to read
    char extrabuf[65536];
    struct iovec vec[2];
    size_t writeable = writeable_bytes();
    vec[0].iov_base = begin() + writeIndex_;
    vec[0].iov_len = writeable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;
    // when there is enough space in this buffer, don't read into extrabuf.
    // when extrabuf is used, we read 128k-1 bytes at most.
    const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = sockets::readv(fd, vec, iovcnt);
    if (n < 0) {
        *savedErrno = errno;
    } else if (static_cast<const size_t>(n) <= writeable) {
        writeIndex_ += n;
    } else {
        writeIndex_ = buf_.size();
        append(extrabuf, n - writeable);
    }
    return n;
}