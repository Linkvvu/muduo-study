#if !defined(MUDUO_BASE_THREADLOCAL_H)
#define MUDUO_BASE_THREADLOCAL_H

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo {

template <typename T>
class threadLocal : private boost::noncopyable {
public:
	// when T is a incomplete type, compiling will occur error.
    static_assert(sizeof(T) > 0, "T must be a complete type");

    threadLocal() {
        pthread_key_create(&pkey_, &threadLocal::destructor);
    }

    ~threadLocal() {
    // 删除TSD(thread-sepcific data)所绑定的key及其相关的数据结构(注意:不会导致"pthread_key_create"中的destructor的调用, 故"pthread_key_delete"一般应该被放在"pthread_key_create"中指定destructor中调用)
        pthread_key_delete(pkey_);
    }

    T& value() {
        T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));
        if (perThreadValue == nullptr) {
            T* new_obj = new T;
            pthread_setspecific(pkey_, new_obj);
            perThreadValue = new_obj;
        }
        return *perThreadValue;
    }    

private:
    static void destructor(void* x) {
        T* perThreadValue = static_cast<T*>(x);
        delete perThreadValue;
    }

private:
    pthread_key_t pkey_;
};

} // namespace muduo 

#endif // MUDUO_BASE_THREADLOCAL_H
