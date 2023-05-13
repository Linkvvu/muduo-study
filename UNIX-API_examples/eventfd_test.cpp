#include <sys/eventfd.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

int create_eventfd() {
    int fd = ::eventfd(100, EFD_CLOEXEC | EFD_NONBLOCK);
    if (fd == -1) {
        std::cerr << "Fail to create eventfd: " << std::strerror(errno) << std::endl;
        ::exit(EXIT_FAILURE); 
    }
    return fd;
}

int main() {
    int fd = create_eventfd();
    uint64_t buf = 22;
    auto ret = ::read(fd, &buf, sizeof buf);
    if (ret == -1) {
        std::cerr << "Fail to read eventfd: " << std::strerror(errno) << std::endl;
        ::exit(EXIT_FAILURE);
    } else {
        std::cout << "value: " << buf << std::endl;
    }
    return 0;
}