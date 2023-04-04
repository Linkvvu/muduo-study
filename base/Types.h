#if !defined(MUDUO_BASE_TYPES_H)
#define MUDUO_BASE_TYPES_H

#include <string>

namespace muduo {
    using std::string;

    template<typename To, typename From>
    inline To implicit_cast(const From& f) { // 显式的进行隐式转换
        return f;
    }    
} // namespace muduo


#endif // MUDUO_BASE_TYPES_H
