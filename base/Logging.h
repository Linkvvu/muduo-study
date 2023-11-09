#if !defined(MUDUO_BASE_LOGGING_H)
#define MUDUO_BASE_LOGGING_H
#include <base/LogStream.h>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <pthread.h>

namespace muduo {

class Logger {
public:
    using OutputHandler = void(*)(const char* data, size_t len);
    using FlushHandler = void(*)();

    struct SourceFile {
        /* 非类型模板参数，compile-time确定数组大小 */
        /* const char (&arr)[N] 表示 const char数组的引用*/
        /* 避免数组名退化为指针，从而保留数组的大小信息 */
        template <int N>
        SourceFile(const char (&arr)[N])
            : data_(arr)
            , size_(N-1)
        {
            const char* boundary = std::strrchr(arr, '/');
            if (boundary) {
                data_ = boundary + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        const char* data_;
        int size_;
    };

    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        FATAL,
    };

    /// For INFO
    Logger(const SourceFile& file, int line);
    /// For WARNING & ERROR & FATAL
    Logger(LogLevel level, const SourceFile& file, int line);
    /// For TRACE & DEBUG
    Logger(LogLevel level, const SourceFile& file, int line, const char* func);
    /// For SYSERR & SYSFATAL
    Logger(const SourceFile& file, int line, bool fatal);
    /// conduct output operation now
    ~Logger();

    base::LogStream& GetStream()
    { return impl_.stream_; }

private:
    class LoggerImpl {
        using time_point = std::chrono::system_clock::time_point;
    public:
        LoggerImpl(LogLevel level, int line, const SourceFile& file, int error_num);
        
        void FormatCurTime() {
            const auto now = std::chrono::system_clock::now();
            const time_t time = std::chrono::system_clock::to_time_t(now);
            const auto tm = std::localtime(&time);
            std::ostringstream out_stream;
            // year-month-day hour:minute:second
            out_stream << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
            const auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            // get 3 bit ms
            auto ms = now_ms.time_since_epoch() % 1000;
            out_stream << '.' << std::setfill('0') << std::setw(3) << ' ';

            // append to stream
            stream_ << out_stream.str();
        }

        void FormatTid() {
            stream_ << "Tid=" << ::pthread_self() << ' ';
        }

        void FormatLevel();

        void Finish();

        /* public data members */
        base::LogStream stream_;
        LogLevel level_;
        SourceFile file_;
        int line_;
    };

private:
    LoggerImpl impl_;
};

/* declare global config */
extern Logger::LogLevel g_logLevel; /* set by environment variable */


inline Logger::LogLevel GetLoglevel()
{
    return g_logLevel;
}

extern void SetOutputHandler(Logger::OutputHandler handler, bool escape = true);
extern void SetFlushHandler(Logger::FlushHandler handler);

extern const char* strerror_thread_safe(int errnum);


/* logging macros */
#define LOG_TRACE                               \
if (muduo::GetLoglevel() <= muduo::Logger::LogLevel::TRACE)   \
    muduo::Logger(muduo::Logger::TRACE, __FILE__, __LINE__, __func__).GetStream()

#define LOG_DEBUG                               \
if (muduo::GetLoglevel() <= muduo::Logger::LogLevel::DEBUG)   \
    muduo::Logger(muduo::Logger::DEBUG, __FILE__, __LINE__, __func__).GetStream()

#define LOG_INFO                                \
if (muduo::GetLoglevel() <= muduo::Logger::Logger::INFO)      \
    muduo::Logger(__FILE__, __LINE__).GetStream()

#define LOG_WARN    \
    muduo::Logger(muduo::Logger::WARNING, __FILE__, __LINE__).GetStream()

#define LOG_ERROR   \
    muduo::Logger(muduo::Logger::ERROR, __FILE__, __LINE__).GetStream()

#define LOG_FATAL   \
    muduo::Logger(muduo::Logger::FATAL, __FILE__, __LINE__).GetStream()

#define LOG_SYSERR  \
    muduo::Logger(__FILE__, __LINE__, false).GetStream()

#define LOG_SYSFATAL    \
    muduo::Logger(__FILE__, __LINE__, true).GetStream()

} // namespace muduo 

#endif // MUDUO_BASE_LOGGING_H
