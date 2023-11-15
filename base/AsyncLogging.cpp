#include <muduo/base/AsyncLogging.h>
#include <muduo/base/LogFile.h>
#include <iomanip>  // std::put_time

using namespace muduo;
using namespace muduo::base;

AsyncLogger::AsyncLogger(const std::string& basename, off_t roll_size, seconds flush_interval)
    : basename_(basename)
    , rollSize_(roll_size)
    , flushInterval_(flush_interval)
    , currentBuffer_(std::make_unique<Buffer>())
    , nextBuffer_(std::make_unique<Buffer>())
{
    buffers_.reserve(16);
}

AsyncLogger::~AsyncLogger() {
    Stop();
}

void AsyncLogger::Append(const char* logline, size_t size) {
    std::lock_guard<std::mutex> guard(mutex_);
    if (currentBuffer_->Avail() > size) {
        currentBuffer_->Append(logline, size);
    } else {
        buffers_.push_back(std::move(currentBuffer_));  // current buffer is full, add to the verctor
        if (nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        } else {
            // rarely happend
            // in this case, means the producer is too fast, and the consumer has no enough time to deal with it
            currentBuffer_.reset(new Buffer());
        }

        assert(currentBuffer_->Avail() > size);  // assert buffer having enough space to accommodate new logs
        currentBuffer_->Append(logline, size);

        nonEmptyCV_.notify_one();   // buffers is non-empty now, notifies back-thread
    }
}

void AsyncLogger::ThreadFunc() {
    assert(running_ == true);
    /* preparatory job  */
    LogFile log_file(basename_, rollSize_, false);    
    BufferPtr newBuffer1 = std::make_unique<Buffer>();
    BufferPtr newBuffer2 = std::make_unique<Buffer>();
    std::vector<BufferPtr> buffers_to_write;    // For shorten the critical-section, make Producer and consumer to log concurrently
    buffers_to_write.reserve(16);

    while (running_) {
        assert(newBuffer1 && newBuffer1->GetLength() == 0);
        assert(newBuffer2 && newBuffer2->GetLength() == 0);
        assert(buffers_to_write.empty());

        {   // critical-section
            std::unique_lock<std::mutex> guard(mutex_);
            std::cv_status status = std::cv_status::no_timeout;
            while (!buffers_.empty() && status == std::cv_status::no_timeout) {
                status = nonEmptyCV_.wait_for(guard, flushInterval_);
            }
            buffers_.push_back(std::move(currentBuffer_));
            buffers_to_write.swap(buffers_);
            currentBuffer_ = std::move(newBuffer1);
            if (!nextBuffer_) {
                nextBuffer_ = std::move(newBuffer2);
            }
        }   // critical-section
        
        assert(buffers_to_write.empty() == false);

        if (buffers_to_write.size() > 25) {
            std::ostringstream oss;
            const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            oss << "Dropped log messages at " << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S")
                << ", " << buffers_to_write.size() - 2 << "larger buffers\n";
            
            std::string msg = oss.str();
            std::fputs(msg.c_str(), stderr);            // output to stand-error
            log_file.Append(msg.c_str(), msg.size());   // output to log-file

            buffers_to_write.erase(buffers_to_write.begin()+2, buffers_to_write.end());
        }

        for (const auto& buf : buffers_to_write) {
            log_file.Append(buf->Data(), buf->GetLength());
        }

        if (buffers_to_write.size() > 2) {
            // drop non-bzero-ed buffers, avoid trashing
            buffers_to_write.resize(2);
        }

        assert(!newBuffer1);
        assert(!buffers_to_write.empty());
        newBuffer1 = std::move(buffers_to_write.back());
        buffers_to_write.pop_back();
        newBuffer1->Reset();

        if (!newBuffer2)
        {
            assert(!buffers_to_write.empty());
            newBuffer2 = std::move(buffers_to_write.back());
            buffers_to_write.pop_back();
            newBuffer2->Reset();
        }

        buffers_to_write.clear();
        log_file.Flush();
    }   // while
    log_file.Flush();
}

