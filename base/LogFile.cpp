#include <base/LogFile.h>
#include <cassert>
#include <fstream>
#include <ctime>    // localtime
#include <unistd.h> // getpid

using namespace muduo::base;

class LogFile::File {
    static const int kIO_BufferSize = 64 * 1024; 
public:
    explicit File(const std::string& name)
        : ofs_(name, std::ios::app|std::ios::binary)
        , fileName_(name)
    {
        assert(ofs_.is_open());
    }

    ~File() {
        ofs_.flush();
        ofs_.close();
    }

    void Append(const char* data, size_t size) {
        ofs_.write(data, size);
        if (!ofs_.good()) {
            std::fprintf(stderr, "LogFile::File::Append() failed, (file name:%s)", fileName_.c_str());
            ofs_.clear();
        }
    }

    int Written()
    { return ofs_.tellp(); }

    void Flush() {
        ofs_.flush();
        if (!ofs_.good()) {
            std::fprintf(stderr, "LogFile::File::Flush() failed, (file name:%s)", fileName_.c_str());
            ofs_.clear();
        }
    }

private:
    std::ofstream ofs_; // std::ofstream uses buffers internally to improve performance
    std::string fileName_;
};

const int LogFile::kPollPerSeconds; // define

LogFile::LogFile(const std::string& basename, off_t roll_size, bool thread_safe
    , int flushInterval, int checkEveryN)
    : basename_(basename)
    , rollSize_(roll_size)
    , flushInterval_(flushInterval)
    , checkEveryN_(checkEveryN)
    , mutex_(thread_safe ? std::make_unique<std::mutex>() : nullptr)
{
    assert(basename_.find('/') == std::string::npos);
    RoolFile();
}

LogFile::~LogFile() = default;

bool LogFile::RoolFile() {
    seconds now(0);
    std::string file_name = GetLogFileName(basename_, &now);
    seconds start_of_period = now / kPollPerSeconds * kPollPerSeconds;

    if (now > lastRoll_) {
        /* update time */
        lastRoll_ = now;
        lastFlush_ = now;
        latestStartOfPeriod_ = start_of_period;
        /* open a new file(roll) */
        file_.reset(new LogFile::File(file_name));
        return true;
    }
    return false;
}

void LogFile::Append(const char* logline, size_t size) {
    if (mutex_) {
        std::lock_guard guard(*mutex_);
        AppendUnlocked(logline, size);
    } else {
        AppendUnlocked(logline, size);
    }
}

void LogFile::Flush() {
    seconds now = std::chrono::duration_cast<seconds>(std::chrono::system_clock::now().time_since_epoch());
    if (mutex_) {
        std::lock_guard<std::mutex> guard(*mutex_);
        file_->Flush();
        lastFlush_ = now;
    } else {
        file_->Flush();
        lastFlush_ = now;
    }
}

void muduo::base::LogFile::AppendUnlocked(const char* logline, size_t size) {
    file_->Append(logline, size);
    if (file_->Written() >= rollSize_) {
        RoolFile();
    } else {
        ++count_;
        if (count_ >= checkEveryN_) {
            count_ = 0;
            seconds now = std::chrono::duration_cast<seconds>(std::chrono::system_clock::now().time_since_epoch());
            seconds current_start_of_period = now / kPollPerSeconds * kPollPerSeconds;
            if (current_start_of_period != latestStartOfPeriod_) {   // has arrived specified period
                RoolFile();
            } else if (now - lastFlush_ > flushInterval_) {         // timeout - has arrived specified flush interval
                Flush();
            } 
        }
    }
}

std::string LogFile::GetLogFileName(const std::string& basename, seconds* now) {
    /* Assign the current time to the pointer 'now' */
    std::chrono::system_clock::time_point now_tp = std::chrono::system_clock::now();
    *now = std::chrono::duration_cast<seconds>(now_tp.time_since_epoch());
    time_t tt = std::chrono::system_clock::to_time_t(now_tp);

    /* Format file-name */
    std::string file_name;
    file_name.reserve(basename.size() + 64);
    file_name = basename;

    /* Format time */
    char timebuf[32] = {0};
    std::strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", std::localtime(&tt));
    file_name += timebuf;

    /* Append PID to file name*/
    char pidbuf[32];
    std::snprintf(pidbuf, sizeof pidbuf, "%d", ::getpid());
    file_name += pidbuf;
    file_name += ".log";
    return file_name;
}

