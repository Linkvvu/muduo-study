#if !defined(MUDUO_BASE_CURRENTTHREAD_H)
#define MUDUO_BASE_CURRENTTHREAD_H

namespace muduo {
namespace currentThread {
    // internal
    // __thread是GCC内置的线程局部变量
    // __thread变量每一个线程有一份独立实体，各个线程的值互不干扰
    // 只能用于POD类型，即不带自定义的构造、拷贝、赋值、析构、virtual func的类型，可按位Copy
    // 不能动态运行时初始化，即使用new关键字赋值
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
