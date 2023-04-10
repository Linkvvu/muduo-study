#include <muduo/base/threadLocal.h>
#include <muduo/base/currentThread.h>
#include <muduo/base/Thread.h>

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

muduo::threadLocal<Test> obj1;
muduo::threadLocal<Test> obj2;

void print() {
    printf("tid=%d, obj1 %p, name=%s\n", currentThread::tid(), &obj1.value(), obj1.value().name().c_str());
    printf("tid=%d, obj2 %p, name=%s\n", currentThread::tid(), &obj2.value(), obj2.value().name().c_str());
}

void threadFunc() {
    print();
    obj1.value().set_name("chanded 1");
    obj2.value().set_name("changed 2");
    print();
}

int main() {
    obj1.value().set_name("main_one");
    print();
    muduo::Thread t1(threadFunc, "extra thread 1");
    t1.start();
    t1.join();
    obj2.value().set_name("main_two");
    print();

    pthread_exit(0);
}