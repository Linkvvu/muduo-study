#include <muduo/base/Thread.h>
#include <muduo/base/singleton.h>
#include <muduo/base/threadLocal.h>
#include <muduo/base/currentThread.h>

using namespace muduo;

class Test : boost::noncopyable {
public:
    Test() : name_("non-name")
    {
        printf("tid=%d, In [%s] thread, constructing %p\n", currentThread::tid(), currentThread::name(), this);
    }

    ~Test() {
        printf("tid=%d, In [%s] thread, destructing %p\n", currentThread::tid(), currentThread::name(), this);
    }

    const std::string& name() const { return name_; }
    void set_name(const std::string& n) { name_ = n; }

private:
    std::string name_;
};

using singleTest_t = muduo::singleton<muduo::threadLocal<Test>>;

void print() {
    printf("tid=%d, %p name=%s\n", currentThread::tid(), &singleTest_t::instance().value(), singleTest_t::instance().value().name().c_str());
}

void threadFunc(const char* changeTo) {
    print();
    singleTest_t::instance().value().set_name(changeTo);
    sleep(1);
    print();
}

int main() {
    singleTest_t::instance().value().set_name("main one");
    muduo::Thread t1(std::bind(threadFunc, "thread1"), "extra thread 1");
    muduo::Thread t2(std::bind(threadFunc, "thread2"), "extra thread 2");
    t1.start();
    t2.start();
    t1.join();
    print();
    t2.join();
    pthread_exit(0);
}