#include <gtest/gtest.h>
#include <EventLoop.h>
#include <Channel.h>
#include <iostream>
#include <unistd.h>
#include <iostream>
#include <poll.h>

const int stdin_fd = 0;
const int kReadEvent = POLLIN | POLLPRI | POLLRDHUP;

TEST(EventLoopTest, master) {
    muduo::EventLoop event_loop;
    muduo::Channel stdin_chan(&event_loop, stdin_fd);
    stdin_chan.EnableReading();
    stdin_chan.SetReadCallback([&stdin_chan](const auto& rcvTime) {
        GTEST_ASSERT_EQ(0, stdin_chan.Index());
        GTEST_ASSERT_EQ(stdin_fd, stdin_chan.FileDescriptor());
        GTEST_ASSERT_EQ(kReadEvent, stdin_chan.CurrentEvent());
        stdin_chan.REventsToString();
        std::string x;  // echo
        std::cin >> x;
        std::cout << x << std::endl;
    });

    if (event_loop.IsInLoopThread()) {
        event_loop.AssertInLoopThread();
        event_loop.Loop();
    } else {
        std::cerr << "not in the loop thread, abort!" << std::endl;
        abort();
    }
}