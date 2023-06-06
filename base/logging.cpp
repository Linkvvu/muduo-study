#include <muduo/base/logging.h>
#include <muduo/base/timeStamp.h>
#include <muduo/base/logStream.h>
#include <muduo/base/currentThread.h>
#include <iostream>

namespace muduo {

__thread char t_errnobuf[32] = {};
__thread char t_time[64] = {};
__thread time_t t_lastest = 0;

const char* strerror_tl(int savedErrno) {
  return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

logger::LogLevel initLogLevel() {
    return logger::TRACE;
}

logger::LogLevel g_logLevel = initLogLevel(); // define program log level

void set_log_level(const logger::LogLevel level) {
    g_logLevel = level;
}

class logger::Impl {
public:
    using logLevel = logger::LogLevel;
    Impl(logLevel level, int old_errno, const logger::sourceFile& file, int line);
    // void format_time();

    TimeStamp time_;
    logStream stream_;
    logLevel level_;
    int line_;
    logger::sourceFile file_baseName_;
};

inline logStream& logger::stream() { return impl_->stream_; }

const char* LogLevelName[logger::NUM_LOG_LEVELS] =
{
  "\033[1;30m[TRACE]\033[0m",
  "\033[1;35m[DEBUG]\033[0m",
  "\033[1;36m[INFO]\033[0m",
  "\033[1;33m[WARN]\033[0m",
  "\033[1;31m[ERROR]\033[0m",
  "\033[1;41m[FATAL]\033[0m",
};

/// @brief logger::Impl constructor
/// @param level 
/// @param savedError 
/// @param file 
/// @param line 
logger::Impl::Impl(logLevel level, int savedError, const logger::sourceFile& file, int line)
    : time_(TimeStamp::now())
    , stream_()
    , level_(level)
    , line_(line)
    , file_baseName_(file) {
        currentThread::cacheTid();
        stream_ << currentThread::tidString() << " " << LogLevelName[level] << ": ";
        if (savedError != 0) {
            stream_ << muduo::strerror_tl(savedError) << " (errno=" << savedError << ") ";
        }
    }

logger::logger(logger::sourceFile file, int line)   // for logLevel INFO
    : impl_(new Impl(INFO, 0, file, line)) {}

logger::logger(logger::sourceFile file, int line, LogLevel level)
    : impl_(new Impl(level, 0, file, line)) {}

logger::logger(logger::sourceFile file, int line, LogLevel level, const char* func)
    : impl_(new Impl(level, 0, file, line)) {
        impl_->stream_ << func << ' ';
    }

logger::logger(logger::sourceFile file, int line, bool whether_abort)
    : impl_(new Impl(whether_abort ? FATAL : ERROR, errno, file, line)) {}


logger::~logger() {
    impl_->stream_ << " - " << impl_->file_baseName_.name_ << ':' << impl_->line_ << '\n';
    const logStream::buffer_t& ref_buf = stream().buffer(); // 获取当前stream底层缓冲区的buf
    g_output_func(ref_buf.data(), ref_buf.size());  // handle output
    if (impl_->level_ == FATAL) {
        g_flush_func();
        abort();
    }
}

/// default handler
void default_output_func(const char* data, std::size_t len) {
    std::cout.write(data, len);
}

void default_flush_func() {
    std::cout.flush();
}

// define class static data 
logger::output_func logger::g_output_func = default_output_func;
logger::flush_func logger::g_flush_func = default_flush_func;





} // namespace muduo 