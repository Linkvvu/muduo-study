#include <muduo/base/Thread.h>
#include <muduo/base/currentThread.h>

#include <string>
#include <functional>
#include <stdio.h>

void threadFunc() {
    printf("tid=%d threadName: %s\n", muduo::currentThread::tid(), muduo::currentThread::t_threadName);
}

void threadFunc2(int x) {
    printf("tid=%d, x=%d threadName: %s\n", muduo::currentThread::tid(), x, muduo::currentThread::name());
}

class Foo {
public:
    explicit Foo(double x)
        : x_(x) {}

    void memberFunc() {
        printf("tid=%d, Foo::x_=%f threadName: %s\n", muduo::currentThread::tid(), x_, muduo::currentThread::name());
    }

    void memberFunc2(const std::string& text) {
        printf("tid=%d, Foo::x_=%f, text=%s threadName: %s\n", muduo::currentThread::tid(), x_, text.c_str(), muduo::currentThread::name());
    }

private:
    double x_;
};

int main() {
    printf("pid=%d, tid=%d\n", ::getpid(), muduo::currentThread::tid());

    muduo::Thread t1(threadFunc);
    t1.start();
    t1.join();

    muduo::Thread t2(std::bind(threadFunc2, 42),
        "thread for free function with argument");
    t2.start();
    t2.join();

    Foo foo(87.53);
    muduo::Thread t3(std::bind(&Foo::memberFunc, &foo),
        "thread for member function without argument");
    t3.start();
    t3.join();

    muduo::Thread t4(std::bind(&Foo::memberFunc2, std::ref(foo),
        "thread for member function with argument"));
    t4.start();
    t4.join();

    muduo::Thread t5([&foo]() {foo.memberFunc2("thread for member function with argument by lambda");}, "lambda thread");
    t5.start();
    t5.join();
    printf("number of created threads %d\n", muduo::Thread::numCreated());
}
