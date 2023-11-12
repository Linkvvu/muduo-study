#include <base/AsyncLogging.h>
#include <base/Logging.h>

#include <iostream>
#include <sys/resource.h>
#include <unistd.h>

using namespace std::chrono;

off_t kRollSize = 500 * 1000 * 1000;

muduo::AsyncLogger* g_asyncLog = NULL;

void asyncOutput(const char* msg, size_t len) {
    g_asyncLog->Append(msg, len);
}

void bench(bool longLog) {
    muduo::Logger::SetOutputHandler(asyncOutput, false);

    int cnt = 0;
    const int kBatch = 1000;
    std::string empty = " ";
    std::string longStr(3000, 'X');
    longStr += " ";

    for (int t = 0; t < 30; ++t) {
        const auto begin = steady_clock::now();
        for (int i = 0; i < kBatch; ++i) {
            LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz "
                << (longLog ? longStr : empty)
                << cnt;
            ++cnt;
        }
        const auto end = steady_clock::now();
        std::cout << (duration_cast<milliseconds>(end - begin) * 1000000 / kBatch).count() << std::endl;

        // std::this_thread::sleep_for(nanoseconds(500 * 1000 * 1000));
    }
}

int main(int argc, char* argv[]) {
    {
        // set max virtual memory to 2GB.
        size_t kOneGB = 1000 * 1024 * 1024;
        ::rlimit rl = { 2 * kOneGB, 2 * kOneGB };
        ::setrlimit(RLIMIT_AS, &rl);
    }

    printf("pid = %d\n", getpid());

    char name[256] = { '\0' };
    strncpy(name, argv[0], sizeof name - 1);
    muduo::AsyncLogger log(::basename(name), kRollSize);
    log.Start();
    g_asyncLog = &log;

    bool longLog = argc > 1;
    bench(longLog);

    // To wait for back-thread to finish writing
    std::this_thread::sleep_for(nanoseconds(500 * 1000 * 1000));
}

