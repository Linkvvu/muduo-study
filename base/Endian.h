#if !defined(MUDUO_BASE_ENDIAN_H)
#define MUDUO_BASE_ENDIAN_H

#include <boost/endian.hpp>
#include <type_traits>
namespace muduo {
namespace base {
namespace endian {
    
                        /* native To target */
template <typename Integer>
std::enable_if_t<std::is_integral<Integer>::value, Integer> NativeToBig(Integer i) {
    return boost::endian::native_to_big(i);
}

template <typename Integer>
std::enable_if_t<std::is_integral<Integer>::value, Integer> NativeToLittle(Integer i) {
    return boost::endian::native_to_little(i);
}


                        /* convert to native */
template <typename Integer>
std::enable_if_t<std::is_integral<Integer>::value, Integer> BigToNative(Integer big) {
    return boost::endian::big_to_native(big);
}

template <typename Integer>
std::enable_if_t<std::is_integral<Integer>::value, Integer> LittleToNative(Integer little) {
    return boost::endian::little_to_native(little);
}

} // namespace endian     
} // namespace base 
} // namespace muduo 


#endif // MUDUO_BASE_ENDIAN_H
