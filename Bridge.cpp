#include <Bridge.h>
#include <logger.h>
#include <Channel.h>
#include <EventLoop.h>
#include <unistd.h>
#include <sys/eventfd.h>

using namespace muduo;

namespace muduo::detail {

int CreateEventFd() {
    int ret = ::eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK);
    if (ret == -1) {
        std::cerr << "Failed to create eventfd, detail: " << strerror_thread_safe(errno) << std::endl;
        abort();
    }
    return ret;
}

} // namespace muduo::detail 

Bridge::Bridge(EventLoop* loop)
    : owner_(loop)
    , chan_(std::make_unique<Channel>(loop, CreateEventFd()))
{
    chan_->EnableReading();
    chan_->SetReadCallback(std::bind(&Bridge::HandleWakeUpFdRead, this));
}

Bridge::~Bridge() noexcept {
    chan_->disableAllEvents();
    chan_->Remove();
    ::close(chan_->FileDescriptor());
}

void Bridge::WakeUp() const {
    uint64_t buffer = 1;
    auto n = ::write(chan_->FileDescriptor(), &buffer, sizeof buffer);
    if (n != sizeof buffer) {
        std::cerr << "Bridge::WakeUp() writes " << n << " bytes instead of 8";
    }
}

void Bridge::HandleWakeUpFdRead() const {
    owner_->AssertInLoopThread();
    uint64_t buffer = 1;
    ssize_t n = ::read(chan_->FileDescriptor(), &buffer, sizeof buffer);
    if (n != sizeof buffer) {
        std::cerr << "Bridge::HandleWakeUpFdRead() reads " << n << " bytes instead of 8";
    }
}

