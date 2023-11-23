#include <muduo/base/ThreadPool.h>
#include <muduo/base/Logging.h>

#include <stdio.h>
#include <pthread.h>

void print() {
    printf("tid=%ld\n", ::pthread_self());
}

void longTask(int num) {
    LOG_INFO << "longTask " << num;
    std::this_thread::sleep_for(std::chrono::seconds(3));
}

void test2() {
    LOG_WARN << "Test ThreadPool by stoping early.";
    muduo::ThreadPool pool("ThreadPool");
    pool.SetMaxQueueSize(5);
    pool.Start(3);

    std::thread t(
        [&pool]() {
            for (int i = 0; i < 20; ++i) {
                pool.Run([i]() { longTask(i); });
            }
        }
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    LOG_WARN << "stop pool";
    pool.Stop();  // early stop

    t.join();

    // run() after stop()
    pool.Run(print);
    LOG_WARN << "test2 Done";
}

int main() {
    test2();
}