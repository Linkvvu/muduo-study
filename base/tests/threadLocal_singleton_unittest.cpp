#include <muduo/base/threadLocal_singleton.h>
#include <muduo/base/currentThread.h>
#include <muduo/base/Thread.h>

using namespace muduo;

class Test : private boost::noncopyable {
public:
    Test() {
        printf("tid=%d, in thread [%s] constructing %p\n", currentThread::tid(), currentThread::name(), this);
    }

    ~Test() {
        printf("tid=%d, in thread [%s] destructing %p\n", currentThread::tid(), currentThread::name(), this);
    }

    const std::string& name() const { return name_; }
    void set_name(const std::string& n) { name_ = n; }

private:
    std::string name_;
};

void threadFunc(const char* changeTo) {
    printf("tid=%d[%s], %p name=%s\n", currentThread::tid(), currentThread::name(), &threadLocal_singleton<Test>::instance(), threadLocal_singleton<Test>::instance().name().c_str());
    threadLocal_singleton<Test>::instance().set_name(changeTo);
    printf("tid=%d[%s], %p name=%s\n", currentThread::tid(), currentThread::name(), &threadLocal_singleton<Test>::instance(), threadLocal_singleton<Test>::instance().name().c_str());
}

int main() {
    threadLocal_singleton<Test>::instance().set_name("main_one");
    Thread t1(std::bind(threadFunc, "thread_1"), "extra thread 1");
    Thread t2(std::bind(threadFunc, "thread_2"), "extra thread 2");
    t1.start();
    t2.start();
    t1.join();
    t2.join();
    printf("tid=%d[%s], %p name=%s\n", currentThread::tid(), currentThread::name(), &threadLocal_singleton<Test>::instance(), threadLocal_singleton<Test>::instance().name().c_str());
    pthread_exit(EXIT_SUCCESS); // 要想调用pthread_key_create()中注册的deleter，则主线程必须调用pthread_exit().否则，若主线程执行return将不会调用deleter
}