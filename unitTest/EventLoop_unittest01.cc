#include <gtest/gtest.h>

#include <EventLoop.h>
#include <iostream>

TEST(EventLoopTest, master) {
    muduo::EventLoop event_loop;
    if (event_loop.IsInLoopThread()) {
        event_loop.AssertInLoopThread();
        event_loop.Loop();
    }
}