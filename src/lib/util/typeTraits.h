#ifndef UTIL_TRAITS_H
#define UTIL_TRAITS_H

#include <cstdlib>

namespace util {

/**
 * Allows for the extraction of type information.
 *
 * This is especially useful when we need to determine the base type and count of an array.
 *
 * ~~~~~~~~~~~~~~{.cpp}
 * typedef T util::BaseType<int[5]>::Type;      // T == int
 * size_t n = util::BaseType<int>::Count;       // n == 1
 * size_t m = util::BaseType<int[5]>::Count;    // m == 5
 * ~~~~~~~~~~~~~~
 */
template <class T>
struct BaseType
{
    BaseType(const T& obj) { }
    typedef T Type;
    enum { Count = 1 };
};

template <class T, size_t N>
struct BaseType<T[N]>
{
    BaseType(const T& obj) { }
    typedef T Type;
    enum { Count = N };
};

}

#endif

