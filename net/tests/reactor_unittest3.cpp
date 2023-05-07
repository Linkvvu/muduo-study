/// 将定时器融入event-loop
#include <muduo/net/eventLoop.h>
#include <muduo/net/channel.h>
#include <muduo/base/logging.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <cstring>
using namespace muduo;

net::event_loop* main_loop;
int timerfd;

void timeout(TimeStamp receiveTime) {
	printf("Timeout!\n");
	uint64_t howmany;
    ::read(timerfd, &howmany, sizeof howmany);
    LOG_INFO << "howmany: " << howmany;
    main_loop->quit();
}

int main() {
    muduo::net::event_loop loop;
    main_loop = &loop;
    timerfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    net::channel timer_chan(main_loop, timerfd);
    struct itimerspec howlong;
    std::memset(&howlong, 0, sizeof howlong);
    howlong.it_value.tv_sec = 3;
    auto res = timerfd_settime(timerfd, 0, &howlong, nullptr);
    if (res != 0) {
        LOG_SYSFATAL << "timerfd_settime";
    }
    
    timer_chan.enableReading(); 
    timer_chan.setReadCallback(timeout);
    main_loop->loop();
    timer_chan.disableAllEvents();
    timer_chan.remove();
    ::close(timerfd);
}