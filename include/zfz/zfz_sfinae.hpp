/*********************************************************** 
* Date: 2018-03-29 
* 
* Author: 牟韵 
* 
* Email: mouyun1115@163.com 
* 
* Module: SFINAE 
* 
* Brief: 强大的模板元编程 
* 
* Note: 
* 
* CodePage: UTF-8 
************************************************************/ 
#ifndef __ZFZ_SFINAE_HPP_BY_MOUYUN_2018_03_29__
#define __ZFZ_SFINAE_HPP_BY_MOUYUN_2018_03_29__

#include <type_traits>
//#include <utility>

namespace zfz
{
namespace SFINAE
{

//------------------------------下列设施可以在编译期判定对象是否可以比较大小,主要用于list排序-----------------------------------//

// 升序感知 
template<typename T, typename = decltype(std::declval<T>() < std::declval<T>())>
std::true_type less_than_helper(const T&);
std::false_type less_than_helper(...);
template<typename T> using less_than = decltype(less_than_helper(std::declval<T>()));

// 降序感知 
template<typename T, typename = decltype(std::declval<T>() > std::declval<T>())>
std::true_type greater_than_helper(const T&);
std::false_type greater_than_helper(...);
template<typename T> using greater_than = decltype(greater_than_helper(std::declval<T>()));

// 实现 
template<typename T, typename std::enable_if<less_than<T>::value && !greater_than<T>::value, int>::type = 0>
static inline bool compare_t(const T &t1, const T &t2)
{
    return t1 < t2;
}
template<typename T, typename std::enable_if<greater_than<T>::value && !less_than<T>::value, int>::type = 0>
static inline bool compare_t(const T &t1, const T &t2)
{
    return t1 > t2;
}
template<typename T, typename std::enable_if<less_than<T>::value && greater_than<T>::value, int>::type = 0>
static inline bool compare_t(const T &t1, const T &t2)
{
    return t1 < t2; // 小于比较和大于比较都支持时,优先使用小于比较 
}
template<typename T1, typename T2>
static inline bool compare_t(const T1 &t1, const T2 &t2)
{
    return false; // 对象不可比较大小时返回false,防止list进行无谓的排序交换操作 
}

//----------------------------------------------------------------------------------------------------------------//

//------------------------下列设施可以在编译期判定对象是否提供了void clear()函数,主要用于对象池---------------------------//
template<typename T>
class clear_function_sensor
{
public:
    template<typename U, void (U::*)()> class matcher;
    template<typename U> static char helper(matcher<U, &U::clear>*);
    template<typename U> static int helper(...);
    enum { value = sizeof(helper<T>(nullptr)) == sizeof(char) };
};

template <bool> class clear_impl;
template<> class clear_impl<true>
{
public:
    template<typename T> inline static void clear(T *p_t) {p_t->clear();}
};
template<> class clear_impl<false>
{
public:
    template<typename T> inline static void clear(T *p_t) {}
};

template<typename T> inline void clear_object(T *p_t)
{
    clear_impl<clear_function_sensor<T>::value>::clear(p_t);
}

} // namespace SFINAE
} // namespace zfz

#endif