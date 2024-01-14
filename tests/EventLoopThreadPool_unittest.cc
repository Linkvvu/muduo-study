#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <muduo/EventLoop.h>
#include <muduo/EventLoopThread.h>
#include <muduo/EventLoopThreadPool.h>
#include <thread>
#include <cstdio>
#include <cassert>
#include <unistd.h>

using namespace muduo;

class Mock {
public:
    MOCK_METHOD(void, print, (EventLoop* p));
    MOCK_METHOD(void, init, (EventLoop*));
};

class EventLoopThreadPoolTestFixture : public testing::Test {
public:
    // 在test suite的第一个测试开始前调用
    static void SetUpTestSuite() {
        ::printf("main(): pid = %d, tid = %ld, loop = %p\n", ::getpid(), ::pthread_self(), &loop);
        loop->RunAfter(std::chrono::seconds(10), std::bind(&EventLoop::Quit, loop));
    }

    // 在test suite的最后一个测试结束后调用
    static void TearDownTestSuite() {
        loop->Loop();
    }

    EventLoop* GetBaseLoop()
    { return loop; }

private:
    static EventLoop* loop;
};

EventLoop* EventLoopThreadPoolTestFixture::loop = muduo::CreateEventLoop().get();  // define class-static data member

TEST_F(EventLoopThreadPoolTestFixture, SingleThread) {
    Mock mock;
    EXPECT_CALL(mock, init(this->GetBaseLoop())).Times(1);
    EXPECT_CALL(mock, print(this->GetBaseLoop())).Times(0);

    EventLoopThreadPool model(this->GetBaseLoop(), "single");
    model.SetPoolSize(0);
    model.SetThreadInitCallback([&mock](EventLoop* loop) {
        mock.init(loop);
    });
    model.BuildAndRun();
    
    ASSERT_EQ(model.GetNextLoop(), this->GetBaseLoop());
    ASSERT_EQ(model.GetNextLoop(), this->GetBaseLoop());
    ASSERT_EQ(model.GetNextLoop(), this->GetBaseLoop());

}

TEST_F(EventLoopThreadPoolTestFixture, AnotherThread) {
    Mock mock;
    EventLoopThreadPool model(this->GetBaseLoop(), "another");

    // Can`t perform following statement Beacuse 'io-thread loop instance' is not created now
    // EXPECT_CALL(mock, init(model.GetNextLoop())).Times(1);
    model.SetPoolSize(1);
    model.SetThreadInitCallback([&mock](EventLoop* loop) {
        mock.init(loop);
    });
    model.BuildAndRun();

    // Can`t perform following statement Beacuse 'init callback' is executed as soon as it's created
    // EXPECT_CALL(mock, init(model.GetNextLoop())).Times(1);
    EXPECT_CALL(mock, print(model.GetNextLoop())).Times(1);
    
    EventLoop* nextLoop = model.GetNextLoop();
    nextLoop->RunAfter(std::chrono::seconds(2), std::bind(&Mock::print, &mock, nextLoop));

    ASSERT_NE(nextLoop, this->GetBaseLoop());
    ASSERT_EQ(nextLoop, model.GetNextLoop());
    ASSERT_EQ(nextLoop, model.GetNextLoop());
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

TEST_F(EventLoopThreadPoolTestFixture, ThreeThreads) {
    Mock mock;
    EventLoopThreadPool model(this->GetBaseLoop(), "three");
    model.SetPoolSize(3);
    model.SetThreadInitCallback(std::bind(&Mock::init, &mock, std::placeholders::_1));
    model.BuildAndRun();
    
    EventLoop* first_loop = model.GetNextLoop();
    EventLoop* second_loop = model.GetNextLoop();
    EventLoop* third_loop = model.GetNextLoop();

    EXPECT_CALL(mock, print(first_loop)).Times(1);
    EXPECT_CALL(mock, print(second_loop)).Times(0);
    EXPECT_CALL(mock, print(third_loop)).Times(0);

    first_loop->RunInEventLoop(std::bind(&Mock::print, &mock, first_loop));
    ASSERT_NE(first_loop, this->GetBaseLoop());
    ASSERT_EQ(first_loop, model.GetNextLoop());
    ASSERT_NE(first_loop, model.GetNextLoop());
    ASSERT_NE(first_loop, model.GetNextLoop());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));    // Waiting for first-loop is awakened And Run 'Mock::print'
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}