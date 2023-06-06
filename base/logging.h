#if !defined(MUDUO_BASE_LOGGING_H)
#define MUDUO_BASE_LOGGING_H

#include <muduo/base/logStream.h>
#include <boost/noncopyable.hpp>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace muduo {


class logger {
    // for build file-baseName
    class sourceFile { // class logger::sourceFile
    public:
        sourceFile(const char* fileName) : name_(fileName) {
            const char* slash = std::strrchr(fileName, '/'); // 返回指针指向字符串中最后一个指定字符的位置
            if (slash) {
                name_ = slash + 1;
            }
            size_ = std::strlen(name_);                
        }

        const char* name_;
        std::size_t size_;
    };

    class Impl;       // forward declaration  
public:
    using output_func = void(*)(const char* data, std::size_t len);
    using flush_func = void(*)();

    enum LogLevel {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS,
    };

    logger(logger::sourceFile file, int line);
    logger(logger::sourceFile file, int line, LogLevel level);
    logger(logger::sourceFile file, int line, LogLevel level, const char* func);
    logger(logger::sourceFile file, int line, bool whether_abort);
    ~logger();
    logStream& stream(); // 获取当前log对象的stream

    static LogLevel logLevel(); // 获取当前程序log级别

    inline static void setoutput_func(output_func f) {
        g_output_func = f;
    }

    inline static void setflush_func(flush_func f) {
        g_flush_func = f;
    }

private:
    static output_func g_output_func;
    static flush_func g_flush_func;

private:
    std::unique_ptr<Impl> impl_;
}; 

#define LOG_TRACE if (muduo::logger::logLevel() <= muduo::logger::TRACE) \
  muduo::logger(__FILE__, __LINE__, muduo::logger::TRACE, __func__).stream()
#define LOG_DEBUG if (muduo::logger::logLevel() <= muduo::logger::DEBUG) \
  muduo::logger(__FILE__, __LINE__, muduo::logger::DEBUG, __func__).stream()
#define LOG_INFO if (muduo::logger::logLevel() <= muduo::logger::INFO) \
  muduo::logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::logger(__FILE__, __LINE__, muduo::logger::WARN).stream()
#define LOG_ERROR muduo::logger(__FILE__, __LINE__, muduo::logger::ERROR).stream()
#define LOG_FATAL muduo::logger(__FILE__, __LINE__, muduo::logger::FATAL).stream()
#define LOG_SYSERR muduo::logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::logger(__FILE__, __LINE__, true).stream()

extern logger::LogLevel g_logLevel;

extern void set_log_level(const logger::LogLevel level);

inline logger::LogLevel logger::logLevel() {
    return g_logLevel;
} 
} // namespace muduo 

#endif // MUDUO_BASE_LOGGING_H
