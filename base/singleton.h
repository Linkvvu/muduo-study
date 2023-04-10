#if !defined(MUDUO_BASE_SINGLETON_H)
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <cstdlib>

namespace muduo {
    
template <typename T>
class singleton : private boost::noncopyable {
public:
	// when T is a incomplete type, compiling will occur error.
    static_assert(sizeof(T) > 0, "T must be a complete type");

    singleton() = delete;
    singleton(singleton&&) = delete;    
    singleton& operator=(singleton&&) = delete;

    static T& instance() {
        pthread_once(&ponce_, []() { init(); });
        return *handle_;
    }
    
private:
    static void init() {
        handle_ = new T;
        ::atexit(destroy);
    }

    static void destroy() {
        delete(handle_);
    }

private:
    static pthread_once_t ponce_;
    static T* handle_;
};

// define

template <typename T>
pthread_once_t singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

template <typename T>
T* singleton<T>::handle_ = nullptr;

} // namespace muduo 

#endif // MUDUO_BASE_SINGLETON_H
