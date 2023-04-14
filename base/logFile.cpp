#include <muduo/base/logFile.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace muduo {

class logFile::file {
public:
    explicit file(const string& n)
        : ofs_(n, std::ios::app|std::ios::out)
        , buffer_({})
        , writen_(0) {
            assert(ofs_.is_open());
            std::filebuf* underlying_buffer = ofs_.rdbuf(); // 获取当前输出文件流的底层缓冲区
            underlying_buffer->pubsetbuf(buffer_, sizeof buffer_); // 设置底层缓冲区的大小
        }

    ~file() { 
        ofs_.flush();
        ofs_.close();
    }

    void append(const char* logline, const std::size_t len) {
        ofs_.write(logline, len);
        writen_ += len;
    }

    std::size_t writen() const { return writen_; }
    void flush() { ofs_.flush(); }

private:
    std::ofstream ofs_;
    char buffer_[64*1024];
    std::size_t writen_;
};

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
    string filename = getLogFileName(basename_);
    time_t now = time(nullptr);
    // 将当前时间戳调整为当天零点的时间戳，然后保存至start中
    // 例如:
    //      每20秒更换一个新文件写入，即kRollPerSeconds_ = 20
    //      当前时间戳为30， int start = 30/20*20;
    //      此时start就为20
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;
    if (now > lastRoll_) {
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

void logFile::append_impl(const char* logline, int len) {
    file_->append(logline, len); // 向文件输入流缓冲区中写入数据
    if (file_->writen() > rollSize_) { // 若日志文件大小达到rollSize_，则换一个新文件
        rollFile();
    } else {
        if (count_ > kCheckTimesRoll_) { // 如果写入次数大于kCheckTimesRoll_(最大写入次数)，则刷新流缓冲区
            time_t now = time(nullptr);
            time_t cur_period = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (cur_period != startOfPeriod_) { // 当前时间与开时记录时间不是同一天, 换新的日志文件
                rollFile();
            } else if (now - lastFlush_ > flushInterval_) {
                file_->flush();
                lastFlush_ = now;
            }
            count_ = 0;
        } else {
            count_++;
        }
    }
}

std::string logFile::getLogFileName(const string& name) {
    std::ostringstream oss;
    auto std_now = std::chrono::system_clock::now();
    const time_t c_native_now = std::chrono::system_clock::to_time_t(std_now);
    tm now;
    localtime_r(&c_native_now, &now);
    oss << name << std::put_time(&now, "%Y%m%d-%H%M%S") << ".log";
    return oss.str();
}


} // namespace muduo 