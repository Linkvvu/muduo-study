#include <muduo/base/currentThread.h>
#include <muduo/base/singleton.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>
#include <boost/noncopyable.hpp>
#include <iostream>

class Test: boost::noncopyable {
public:
    Test() {
        printf("tid=%d, constructing %p\n", muduo::currentThread::tid(), this);
    }

    ~Test() {
        printf("tid=%d, destructing %p %s\n", muduo::currentThread::tid(), this, name_.c_str());
    }

    const std::string& name() const { return name_; }
    void setName(const muduo::string& n) { name_ = n; }

private:
    std::string name_;
};

void threadFunc() {
    printf("tid=%d, %p name=%s\n",muduo::currentThread::tid(),
        &muduo::singleton<Test>::instance(),
        muduo::singleton<Test>::instance().name().c_str());
    muduo::singleton<Test>::instance().setName("only one [changed]");
}

int main() {
    muduo::singleton<Test>::destroy();
    muduo::Thread t1(threadFunc);
    t1.start();
    t1.join();
    printf("tid=%d, %p name=%s\n",
        muduo::currentThread::tid(),
        &muduo::singleton<Test>::instance(),
        muduo::singleton<Test>::instance().name().c_str()
    );
}