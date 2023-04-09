#if !defined(MUDUO_BASE_THREADPOOL_H)
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Types.h> // for std::string
#include <muduo/base/Thread.h>
#include <muduo/base/condition_variable.h>
#include <boost/noncopyable.hpp>
#include <functional>
#include <memory>
#include <vector>
#include <deque>

namespace muduo {

class threadpool : private boost::noncopyable {
public:
    using Task = std::function<void(void)>; // element type of task queue;
    threadpool(const string& name = string());
    ~threadpool();

    void start(std::size_t numThread);
    void stop();
    void run(const Task& func);

private:
    void runInThread();
    Task take();
    
private:
    string name_;
    bool running_;
    mutexLock mutex_;   // mutex for task queue
    condition_variable notEmptyCV_; // condition for task queue not empty
    std::deque<Task> taskQueue_;
    std::vector<std::unique_ptr<muduo::Thread>> threads_;
};
    
} // namespace muduo 


#endif // MUDUO_BASE_THREADPOOL_H
