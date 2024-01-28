#include <muduo/base/Logging.h>
#include <iostream>


using namespace muduo;
namespace muduo::base::detail {

static inline LogStream& operator<<(LogStream& stream, const Logger::SourceFile& file)
{
    stream.Append(file.data_, file.size_);
    return stream;
}

static inline Logger::LogLevel initLogLevel() {
    if (::getenv("MUDUO_LOG_TRACE"))
        return Logger::TRACE;
    else if (::getenv("MUDUO_LOG_DEBUG"))
        return Logger::DEBUG;
    else
        return Logger::INFO;
}

const char* LevelName[] =
{
    "[TRACE] ",
    "[DEBUG] ",
    "[INFO ] ",
    "[WARN ] ",
    "[ERROR] ",
    "[FATAL] "
};

#define L_RESET         "\033[0m"
#define L_RED           "\033[1;31m"
#define L_GREEN         "\033[1;32m"
#define L_YELLOW        "\033[1;33m"
#define L_BLUE          "\033[1;34m"
#define L_PURPLE        "\033[1;35m"
#define L_CYAN          "\033[1;36m"
#define L_LIGHT_YELLOW  "\033[1;93m"
#define L_PINK          "\033[1;95m"
#define L_LIGHT_CYAN    "\033[1;96m"

const char* LevelNameWithColor[] =
{
    L_LIGHT_CYAN "[TRACE] " L_RESET,
    L_PINK "[DEBUG] " L_RESET,
    L_BLUE "[INFO ] " L_RESET,
    L_LIGHT_YELLOW "[WARN ] " L_RESET,
    L_RED "[ERROR] " L_RESET,
    L_RED "[FATAL] " L_RESET
};

} // namespace muduo::base::detail 


/* define global config */
Logger::LogLevel muduo::g_logLevel = base::detail::initLogLevel(); /* set by environment variable */

namespace muduo {
    thread_local char tl_errnoBuffer[512] /* = nullptr */;
    const char* strerror_thread_safe(int errnum) {
        // the GNU version is used by default
        return strerror_r(errnum, tl_errnoBuffer, 512);
    }

    void defaultOutputHandler(const char* msg, size_t len) {
        std::cout.write(msg, len);
    }

    void defaultFlushHandler() {
        std::cout.flush();
    }

    Logger::OutputHandler g_output_handler = muduo::defaultOutputHandler;
    Logger::FlushHandler g_flush_handler = muduo::defaultFlushHandler;
    bool g_toConsole = true;    // control escape

    inline void OutputColor(bool on)
    { g_toConsole = on; }

    void Logger::SetOutputHandler(Logger::OutputHandler handler, bool use_escape) {
        g_output_handler = handler;
        OutputColor(use_escape);
    }
    
    void Logger::SetFlushHandler(Logger::FlushHandler handler) {
        g_flush_handler = handler;
    }
} // namespace muduo 


void muduo::Logger::LoggerImpl::FormatLevel() {
    if (g_toConsole) {
        stream_ << base::detail::LevelNameWithColor[level_];
    } else {
        stream_ << base::detail::LevelName[level_];
    }
}

inline void muduo::Logger::LoggerImpl::Finish() {
    /* use muduo::base::LogStream& muduo::base::detail::operator<< (...) */
    using namespace muduo::base::detail;
    stream_ << " - " << file_ << ':' << line_ << '\n';
}

Logger::LoggerImpl::LoggerImpl(LogLevel level, int line, const SourceFile& file, int error_num)
    : level_(level)
    , file_(file)
    , line_(line)
{
    FormatLevel();
    FormatCurTime();
    FormatTid();
    if (error_num != 0) {
        stream_ << strerror_thread_safe(error_num) << " (errno=" << error_num << ") ";
    }
}

Logger::Logger(const SourceFile& file, int line) 
    : impl_(INFO, line, file, 0)
    { }

Logger::Logger(LogLevel level, const SourceFile& file, int line)
    : impl_(level, line, file, 0)
    { }

Logger::Logger(const SourceFile& file, int line, bool fatal) 
    : impl_(fatal ? FATAL : ERROR, line, file, errno)
    { }

Logger::Logger(LogLevel level, const SourceFile& file, int line, const char* func)
    : impl_(level, line, file, 0)
{
    impl_.stream_ << "{" << func << "} ";
}

muduo::Logger::~Logger() {
    impl_.Finish();
    const auto& internal_buf = GetStream().GetInternalBuf();
    g_output_handler(internal_buf.Data(), internal_buf.GetLength());
    if (impl_.level_ == FATAL) {
        g_flush_handler();
        abort();
    }
}
