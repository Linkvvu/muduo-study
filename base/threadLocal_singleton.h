#if !defined(MUDUO_BASE_THREADLOCAL_SINGLETON_H)
#define MUDUO_BASE_THREADLOCAL_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <cassert>

#if nullptr

namespace muduo {

template <typename T>
class threadLocal_singleton : private boost::noncopyable{
public:
    static_assert(sizeof(T) > 0, "T must be a complete type");
    
    static T& instance() {
        pthread_once(&ponce_, &threadLocal_singleton<T>::create);

        if (handle_ == nullptr) {
            handle_ = new T;
            pthread_setspecific(pkey_, handle_); // for invoke deleter to release handle_ when thread exit
        }
        return *handle_;
    }

    static T* pointer() {
        return handle_;
    }

private:
    threadLocal_singleton() = delete;
    threadLocal_singleton(threadLocal_singleton&&) = delete;
    threadLocal_singleton& operator=(threadLocal_singleton&&) = delete;

    static void create() {
        pthread_key_create(&pkey_, &threadLocal_singleton<T>::destructor);
        // "进程"结束时调用pthread_key_delete，释放TSD所绑定的key及其相关数据结构
        ::atexit(&threadLocal_singleton<T>::delete_key);
    }

    static void delete_key() {
        pthread_key_delete(pkey_);
    }

    static void destructor(void* obj) {
        assert(handle_ == obj);
        delete handle_;
        handle_ = nullptr;
    }

private:
    static __thread T* handle_;
    static pthread_key_t pkey_;
    static pthread_once_t ponce_;
};

// define class static data

template <typename T>
__thread T* threadLocal_singleton<T>::handle_ = nullptr; // 用编译期常量初始化__thread修饰的static variable 
    
template <typename T>
pthread_once_t threadLocal_singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template <typename T>
pthread_key_t threadLocal_singleton<T>::pkey_;

} // namespace muduo 

#else

namespace muduo {

template <typename T>
class threadLocal_singleton : private boost::noncopyable {
public:
    static_assert(sizeof(T) > 0, "T must be a complete type");

    static T& instance() {
        if (handle_ == nullptr) {
            handle_ = new T;      // assignment-operation, not initialization-operation ok.
            deleter_.set(handle_);
        }
        return *handle_;
    }

    static T* pointer() {
        return handle_;
    }

private:
    threadLocal_singleton() = delete;
    threadLocal_singleton(threadLocal_singleton&&) = delete;
    threadLocal_singleton& operator=(threadLocal_singleton&&) = delete;

    static void destructor(void* obj) {
        assert(obj == handle_);
        delete handle_;
        handle_ = nullptr;
    }

    struct Deleter {
        Deleter() {
            // 设置destructor为该key的删除器, 当线程退出(即pthread_exit)时，删除器会被调用
            // 注意: 总是先调用删除器，再调用[struct Deleter]的destructor
            //       删除器是再thread exit时被调用的，而[struct Deleter]的destructor是离开作用域时、程序或线程结束时的内存释放工作，(若声明为全局变量的情况下)其调用一定晚于key中保存的deleter
            pthread_key_create(&pkey_, &destructor); // 当编译器在[struct Deleter] scope 内找不到符号destructor时，会向外围scope查找，直到 global scope 
        }

        ~Deleter() {
            pthread_key_delete(pkey_);
        }

        void set(T* newObj) {
            assert(pthread_getspecific(pkey_) == nullptr);
            pthread_setspecific(pkey_, newObj);
        }

        pthread_key_t pkey_;
    };

private:
    static __thread T* handle_;
    static Deleter deleter_;
};

// define class static data

template <typename T>
__thread T* threadLocal_singleton<T>::handle_ = nullptr; // 用编译期常量初始化__thread修饰的static variable

template <typename T>
typename threadLocal_singleton<T>::Deleter threadLocal_singleton<T>::deleter_; // call constructor

} // namespace muduo 

#endif // 

#endif // MUDUO_BASE_THREADLOCAL_SINGLETON_H
