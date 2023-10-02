#include <Timer.h>
#include <Channel.h>
#include <EventLoop.h>
#include <TimerQueue.h>
#include <sys/timerfd.h>
#include <unistd.h>

using namespace muduo;

int CreateTimerfd() {
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK|TFD_CLOEXEC);
    if (fd == -1) {
        std::cerr << "Failed to create timerfd, detail: " << strerror_thread_safe(errno) << std::endl;
        abort();
    }
    return fd;
}


Watcher::Watcher(TimerQueue* owner)
    : owner_(owner)
    , watcherChannel_(std::make_unique<Channel>(owner_->Owner(), CreateTimerfd()))
{
    watcherChannel_->EnableReading();
    watcherChannel_->SetReadCallback(std::bind(&Watcher::HandleExpiredTimers, this));
    // watcherChannel_->SetReadCallback([this](Channel::ReceiveTimePoint_t t) {
    //     this->HandleExpiredTimers();
    // });
}

Watcher::~Watcher() noexcept { 
    watcherChannel_->disableAllEvents();
    watcherChannel_->Remove();
    close(watcherChannel_->FileDescriptor());
}

void Watcher::HandleExpiredTimers() {
    ReadTimerfd();
    owner_->HandleExpiredTimers();
}

void Watcher::ReadTimerfd() const {
    uint64_t count = 0; 
    int ret = ::read(watcherChannel_->FileDescriptor(), &count, sizeof count);
    if (ret != sizeof count) {
        std::cerr << "Watcher::ReadTimerfd reads " << ret << " bytes instead of 8" << std::endl;
    }
    // return count;
}
