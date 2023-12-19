#ifndef MUDUO_BASE_ALLOCATOR_CONSTRUCT_H
#define MUDUO_BASE_ALLOCATOR_CONSTRUCT_H

#include <new> // for placement new
#include <type_traits>	// for std::is_trivially_destructible<>

/// The file defines some function that responsible for construction and destruction

namespace muduo {
namespace base {
namespace detail {

template <class _T1, class _T2>
inline void construct(_T1* place, const _T2& __value) {
	new (static_cast<void*>(place)) _T1(__value);  // placement new; 唤起T1::T1(value)
}

template <typename _T1>
inline void construct(_T1* place) {
	new (static_cast<void*>(place)) _T1();
}

/// @brief Version 1: invoke destructor of the specified type object @c _Tp  
/// @tparam _Tp specified type
template <typename _Tp>
inline void destroy(_Tp* ptr) {
	ptr->~_Tp();  // Call its destructor  
}

/// @brief Version 2: invoke destructor of objects within a given range
/// @tparam _Tp forward iterator, required
template <typename ForwardIterator>
inline void destroy(ForwardIterator first, ForwardIterator last) {
	destroy(first, last, ForwardIterator::value_type);
}

template <typename ForwardIterator, typename T>
inline void destroy(ForwardIterator first, ForwardIterator last, T*) {
	destroy_aux(first, last, std::is_trivially_destructible<T>::value);
}

// When the element type has non-trivial destructor
template <class ForwardIterator>
void destroy_aux(ForwardIterator first, ForwardIterator last, std::false_type) {
	// Call the destructor for each element one by one
	for (; first != last; ++first)
		destroy(&*first);
}

// When the element type has trivial destructor
template <class ForwardIterator>
inline void destroy_aux(ForwardIterator, ForwardIterator, std::true_type) {
	// do nothing...
}

/// define normal function to prevent the compiler instantiating destroy<char*>(char*, char*) and so on...
inline void destroy(char*, char*) 		{ /* do nothing */ }
inline void destroy(int*, int*) 		{ /* do nothing */ }
inline void destroy(long*, long*) 		{ /* do nothing */ }
inline void destroy(float*, float*) 	{ /* do nothing */ }
inline void destroy(double*, double*) 	{ /* do nothing */ }

} // namespace detail 
} // namespace base 
} // namespace muduo 

#endif // MUDUO_BASE_ALLOCATOR_CONSTRUCT_H
