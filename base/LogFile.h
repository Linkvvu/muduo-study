#if !defined(MUDUO_BASE_LOG_FILE_H)
#define MUDUO_BASE_LOG_FILE_H

#include <mutex>
#include <memory>
#include <chrono>

namespace muduo {
namespace base {

class LogFile {
    // non-copyable & non-moveable
    LogFile(const LogFile&) = delete;
    LogFile& operator=(const LogFile&) = delete;
    using seconds = std::chrono::seconds;
    class File; // declare
public:
    LogFile(const std::string& basename, off_t roll_size
        , bool thread_safe = true, int flushInterval = 3 /*seconds*/
        , int checkEveryN = 1024);
    ~LogFile();

    void Append(const char* logline, size_t size);    
    void Flush();
    bool RoolFile();

private:
    void AppendUnlocked(const char* logline, size_t size);

private:
    static const int kPollPerSeconds = 60*60*24;
    /// eg. (basename).1970.01.01-00.00.00.(pid).log
    static std::string GetLogFileName(const std::string& basename, seconds* now);

private:
    const std::string basename_;
    const off_t rollSize_;
    const std::chrono::seconds flushInterval_;
    const int checkEveryN_;
    int count_ {0};

    seconds lastRoll_ {0};
    seconds lastFlush_ {0};
    seconds latestStartOfPeriod_ {0};
    
    std::unique_ptr<std::mutex> mutex_;
    std::unique_ptr<LogFile::File> file_;
};

} // namespace base 
} // namespace muduo 

#endif // MUDUO_BASE_LOG_FILE_H
