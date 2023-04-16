#include <muduo/base/logFile.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace muduo {

// non thread-safely
class logFile::file { // class logFile::file
public:
    explicit file(const string& n)
        : ofs_(n, std::ios::app|std::ios::out)
        , buffer_()
        , writen_(0) {
            assert(ofs_.is_open());
            ofs_.rdbuf()->pubsetbuf(buffer_, sizeof buffer_); // 设置底层缓冲区机及大小
                                                              // 若写入内容大于缓冲区大小，则ofstream对象会自动刷新缓冲区
        }

    ~file() {
        ofs_.flush();
        ofs_.close();
    }

    void append(const char* logline, const std::size_t len) {
        ofs_.write(logline, len);   // ofstream::write的实现中没有加锁，故非线程安全函数
        writen_ += len;
    }

    std::size_t writen() const { return writen_; }
    void flush() { ofs_.flush(); }

private:
    std::ofstream ofs_;
    char buffer_[64*1024];
    std::size_t writen_;
}; // class logFile::file

logFile::logFile(const string& name, const std::size_t rollSize, bool thread_saftly, const int flush_interval)
    : basename_(name)
    , rollSize_(rollSize)
    , flushInterval_(flush_interval)
    , count_(0)
    , mutex_(thread_saftly ? new mutexLock : nullptr)
    , startOfPeriod_(0)
    , lastRoll_(0)
    , lastFlush_(0)
    , file_(nullptr) {
        assert(basename_.find('/') == std::string::npos);
        rollFile();
    }

void logFile::rollFile() {
    time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    // 将当前时间戳调整为当天零点的时间戳，然后保存至start中
    // 例如:
    //      每20秒更换一个新文件写入，即kRollPerSeconds_ = 20
    //      当前时间戳为30， int start = 30/20*20;
    //      此时start就为20
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
    if (now > lastRoll_) { // 如果当前时间与上次滚动时间不是同一秒，则更换新的文件
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new file(getLogFileName(basename_)));
    }
}

logFile::~logFile() = default;

void logFile::append(const char* logline, int len) {
    if (mutex_) { // 若客户指定thread-saftly = true则加锁,否则不加锁直接填充缓冲区
        mutexLock_guard locker(*mutex_);
        append_impl(logline, len);
    } else {
        append_impl(logline, len);
    }
}

void logFile::flush() {
    file_->flush();
}

void logFile::append_impl(const char* logline, int len) {
    file_->append(logline, len); // 向文件输入流缓冲区中写入数据
    if (file_->writen() > rollSize_) { // 若日志文件大小达到rollSize_，则换一个新文件
        rollFile();
    } else {
        count_++;
        if (count_ > kCheckTimesRoll_) { // 如果写入次数大于kCheckTimesRoll_(最大写入次数)，则刷新流缓冲区
            time_t now = time(nullptr);
            time_t cur_period = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (cur_period != startOfPeriod_) { // 当前时间与开时记录时间不是同一天, 换新的日志文件
                rollFile();
            } else if (now - lastFlush_ > flushInterval_) {
                file_->flush();
                lastFlush_ = now;
            }
            count_ = 0; // 重置count_，等待下一次检查
        }
    }
}

std::string logFile::getLogFileName(const string& name) {
    std::ostringstream oss;
    auto std_now = std::chrono::system_clock::now();
    const time_t c_native_now = std::chrono::system_clock::to_time_t(std_now);
    tm now;
    localtime_r(&c_native_now, &now); // 获取本地时区时间(thread-safely)
    oss << name << std::put_time(&now, "%Y%m%d-%H%M%S") << ".log";
    return oss.str();
}


} // namespace muduo 