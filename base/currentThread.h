#if !defined(MUDUO_BASE_CURRENTTHREAD_H)
#define MUDUO_BASE_CURRENTTHREAD_H

namespace muduo {
namespace currentThread {
    // internal
    // __thread是GCC内置的线程局部变量
    // __thread变量每一个线程有一份独立实体，各个线程的值互不干扰
    // 只能用于POD类型，即不带自定义的构造、拷贝、赋值、析构、virtual func及virtual base，可按位Copy的类型
    // 不能修饰非POD类型的class，因为无法调用构造\析构函数（POD类型的class的构造与析构的默认操作就是什么都不做，故可被修饰）
    // 不能动态运行时初始化，即使用new关键字赋值，因为每个线程都需要有自己的副本，而 new 在堆上分配的内存是共享的
    // __thread 修饰的变量需要在声明时进行初始化(只能初始化为"编译器常量")
    // __thread 变量只能在(1)函数外部(2)命名空间作用域(3)类内静态数据成员中声明
    extern __thread int t_cachedTid; // 变量定义在 Thread.cpp 文件
    extern __thread char t_tidString[11+1];
    extern __thread const char* t_threadName;

    void cacheTid();
    bool isMainThread();

    // 因为声明为inline，故编译器需要在编译阶段将函数体展开至函数调用处
    // 因为编译器需要知道函数体信息，故将函数定义放在头文件

    // return tid. 若没有缓存tid，则缓存
    inline int tid() {
        if (t_cachedTid == 0) {
            cacheTid(); // 将Tid缓存下来
        }
        return t_cachedTid;
    }

    inline const char* tidString() {
        return t_tidString;
    }

    inline const char* name() {
        return t_threadName;
    }

} // namespace currentThread     
} // namespace muduo

#endif // MUDUO_BASE_CURRENTTHREAD_H
