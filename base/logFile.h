#if !defined(MUDUO_BASE_LOGFILE_H)
#define MUDUO_BASE_LOGFILE_H

// logFile feature:
// 1.每新的一天自动更换日志文件
// 2.当前文件大小超过用户指定的rollSize则滚动文件(即更换新文件)
// 3.设置文件缓冲区，用户可指定每X秒刷新一次缓冲区

#include <muduo/base/Types.h>
#include <muduo/base/mutex.h>
#include <boost/noncopyable.hpp>
#include <memory>

namespace muduo {

class logFile : private boost::noncopyable {
    class file;                     // forward declaration
public:
    explicit logFile(const string& name, const std::size_t rollSize, bool thread_saftly = true, const int flush_interval = 3);
    ~logFile();
    void rollFile();
    void append(const char* logline, int len);
    void append_impl(const char* logline, int len);
    static std::string getLogFileName(const string& name);
private:
    const string basename_;
    const std::size_t rollSize_;    // 日志文件达到rollSize_换一个新文件
    const int flushInterval_;		// 日志写入间隔时间

    int count_;                     // 当count大于kCheckTimesRoll_值时，并且当前为新的一天或到达了日志写入间隔时间，则刷新缓冲区
    std::unique_ptr<mutexLock> mutex_;
    time_t startOfPeriod_;	        // 上次开时记录日志的当天零点时间
    time_t lastRoll_;			    // 上一次滚动日志文件时间
    time_t lastFlush_;		        // 上一次日志写入文件时间
    std::unique_ptr<logFile::file> file_;

    const static int kCheckTimesRoll_ = 1024;       // 当累计向缓冲区中写入了kCheckTimesRoll_条日志时，开时判断是否为新的一天，或是否需要刷新缓冲区
    const static int kRollPerSeconds_ = 60*60*24;   // 一天的总秒数，用于将当前时间戳调整为当天零点
};     

} // namespace muduo 

#endif // MUDUO_BASE_LOGFILE_H
