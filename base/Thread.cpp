#include "muduo/base/currentThread.h"
#include "muduo/base/Exception.h"
#include "muduo/base/Thread.h"

#include <unistd.h>      // for syscall
#include <sys/syscall.h> // for SYS_gettid
#include <cstring>
#include <cassert>
#include <climits>
#include <type_traits>

namespace muduo {
namespace currentThread {
    __thread int t_cachedTid = 0; // 线程真实pid（tid）的缓存，缓存是为了减少::syscall(SYS_gettid)系统调用的次数
    __thread char t_tidString[11+1] = {0}; // tid的字符串形式
    __thread const char* t_threadName = "unknow"; // 线程友好名称

    const bool sameType = std::is_same<int, pid_t>::value;
    static_assert(sameType, "type [int] and type [pid_t] is not same!");
} // namespace currentThread 

namespace detail {
    pid_t gettid() {
        // return ::gettid();
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }

    void afterfork_InChild() {
        muduo::currentThread::t_cachedTid = 0;
        muduo::currentThread::t_threadName = {0};
        muduo::currentThread::t_threadName = "main"; // fork后子进程中只会有一个线程(即使父进程为多线程程序）
                                                     // 且子进程中的该线程是父进程中“调用fork函数的那个线程”的副本
                                                     // 因为fork后子进程只有一个线程，故为主线程 “t_threadName”应置为main
        currentThread::tid(); // 缓存tid
    }

    class threadNameInitializer {
    public:
        threadNameInitializer() {
            muduo::currentThread::t_threadName = "main";
            currentThread::tid();
            pthread_atfork(nullptr, nullptr, afterfork_InChild);
        }
    };

    threadNameInitializer obj_for_init; // 由于该对象为global var，故会在main函数执行前构造
                                        // 因此，该对象的作用是初始化主线程的__thread信息
} // namespace detai 
} // namespace muduo 

namespace muduo::currentThread {
    void cacheTid() {
        if (t_cachedTid == 0) {
            t_cachedTid = muduo::detail::gettid();
            int n = snprintf(t_tidString, sizeof(t_tidString), "%5d", t_cachedTid);
            assert(n == 5);
            (void)n; // 使用一下变量n, 防止release版本编译出现警告
        }
    }

    bool isMainThread() {
        return currentThread::tid() == ::getpid();
    }
} // namespace currentThread 

using namespace muduo;

std::atomic<int> Thread::createdCount(0); // define member data

Thread::Thread(const ThreadFunc& func, const string& name)
    : started_(false)
    , joined_(false)
    , pthreadId_(0)
    , tid_(0)
    , func_(func)
    , name_(name) {
        createdCount++;
    }

Thread::~Thread() {
    if (started_ && !joined_) {
        pthread_detach(pthreadId_);
    }
}

void Thread::start() {
    assert(!started_);
    started_ = true;
    errno = pthread_create(&pthreadId_, nullptr, startThread, this);
    if (errno != 0) {
        //LOG_SYSFATAL << "Failed in pthread_create";
    }
}

int Thread::join() {
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, nullptr);
}

void Thread::runInThread() {
    // 初始化当前新创建线程的__thread信息
    tid_ = currentThread::tid();
    muduo::currentThread::t_threadName = name_.c_str();
    
    try {
        func_();
        muduo::currentThread::t_threadName = "finished_thread";
    } catch (const muduo::Exception& ex) {
        muduo::currentThread::t_threadName = "crashed_thread";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: \n%s\n", ex.stackTrace());
        abort();
    } catch (const std::exception& ex) {
        muduo::currentThread::t_threadName = "crashed_thread";
        fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    } catch (...) {
        muduo::currentThread::t_threadName = "crashed_thread";
        fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
        throw;
    }
}

void* Thread::startThread(void* args) {
    Thread* curThread = static_cast<Thread*>(args);
    curThread->runInThread();
    return nullptr;
}
