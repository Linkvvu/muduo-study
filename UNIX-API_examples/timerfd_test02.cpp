#include <chrono>
#include <iostream>
#include <cstring>
#include <iomanip>
#include <cstdlib>
#include <unistd.h>
#include <sys/timerfd.h>


int main() {
    using namespace std;

    chrono::system_clock::time_point now = chrono::system_clock::now();
    auto t_t = chrono::system_clock::to_time_t(now);
    std::cout << std::put_time(std::localtime(&t_t), "%x %X") << std::endl;

    // 3 seconds later  NOTE: use system clock(realy clock)!!!
    chrono::system_clock::time_point three_s_later = now + chrono::seconds(3);
    auto fd = timerfd_create(CLOCK_REALTIME, TFD_CLOEXEC);
    itimerspec when{};
    when.it_value.tv_sec = static_cast<time_t>(chrono::duration_cast<chrono::seconds>(three_s_later.time_since_epoch()).count());
    when.it_value.tv_nsec = 0;
    // NOTE: use absolute time!!!
    int ret = timerfd_settime(fd, TFD_TIMER_ABSTIME, &when, nullptr);
    if (ret != 0) {
        std::cerr << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    int64_t count = -100;
    ret = ::read(fd, &count, sizeof count);
    if (ret == -1) {
        std::cerr << std::strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "count: " << count << std::endl;
    t_t = chrono::system_clock::to_time_t(chrono::system_clock::now());
    std::cout << std::put_time(std::localtime(&t_t), "%x %X") << std::endl;
}