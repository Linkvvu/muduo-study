#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h>
#include <unistd.h>

using namespace muduo::base;
std::unique_ptr<LogFile> g_logFile;

void outputFunc(const char* msg, size_t len) {
    g_logFile->Append(msg, len);
}

void flushFunc() {
    g_logFile->Flush();
}

int main(int argc, char* argv[]) {
    char name[256] = { '\0' };
    strncpy(name, argv[0], sizeof name-1);
    g_logFile.reset(new LogFile(::basename(name), 200 * 1024));
    muduo::Logger::SetOutputHandler(outputFunc, false);
    muduo::Logger::SetFlushHandler(flushFunc);

    std::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    for (int i = 0; i < 10000; ++i) {
        LOG_INFO << line << i;

        usleep(1000);
    }
}