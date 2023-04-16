#include <muduo/base/logFile.h>
#include <muduo/base/logging.h>
#include <unistd.h>

std::unique_ptr<muduo::logFile> g_logFile;

void outputFunc(const char* msg, std::size_t len) {
    g_logFile->append(msg, len);
}

void flushFunc() {
    g_logFile->flush();
}

int main(int argc, char* argv[]) {
    char name[256] = { '\0' };
    strncpy(name, argv[0], sizeof name - 1);
    g_logFile.reset(new muduo::logFile(::basename(name), 200 * 1024));
    muduo::logger::setoutput_func(outputFunc);
    muduo::logger::setflush_func(flushFunc);

    muduo::string line = "1234567890 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    for (int i = 0; i < 10000; ++i) {
        LOG_INFO << line << i;

        usleep(1000);
    }
}
