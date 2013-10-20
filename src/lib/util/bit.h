#ifndef UTIL_BITS_H
#define UTIL_BITS_H

#include <algorithm>
#include <bitset>
#include <climits>
#include <cstring>

namespace util {

template <typename T>
inline constexpr size_t numBits()
{
    return sizeof(T)*CHAR_BIT;
}

template <typename T>
inline constexpr T setBit(T x, size_t idx)
{
    return x | (static_cast<T>(1) << idx);
}

template <typename T>
inline constexpr T resetBit(T x, size_t idx)
{
    return x & ~(static_cast<T>(1) << idx);
}

template <typename T>
inline constexpr T setBit(T x, size_t idx, size_t val)
{
    return val == 0 ? resetBit(x, idx) : setBit(x, idx);
}

template <typename T>
inline constexpr bool checkBit(T x, size_t idx)
{
    return x & (1 << idx);
}

template <typename T>
inline constexpr bool anyBitsSet(T x)
{
    return x > 0;
}

template <typename T>
inline constexpr bool allBitsSet(T x)
{
    return (~x) == 0;
}

template <typename T>
inline constexpr bool noBitsSet(T x)
{
    return x == 0;
}

template <typename T>
inline constexpr size_t msb(T x)
{
    return (x & 
            (static_cast<T>(1) << (numBits<T>() - 1))) >> (numBits<T>() - 1);
}

template <typename T>
inline constexpr size_t lsb(T x)
{
    return x & 0x1;
}

template <typename T>
inline size_t countLeadingZeroes(T x)
{
    if (x == 0) {
        // __builtin_clz is undefined when x == 0
        return numBits<T>();
    } else {
        // __builtin_clz operates on unsigned ints but T
        // might be of a different type so we need to account for the
        // potential for more leading zeroes.
        return __builtin_clz(x) - (numBits<unsigned int>() - numBits<T>());
    }
}

template <typename T>
inline size_t countTrailingZeroes(T x)
{
    // See comment on countLeadingZeroes
    if (x == 0) {
        return numBits<T>();
    } else {
        return std::min(__builtin_ctz(x),
                static_cast<int>(numBits<T>()));
    }
}

template <typename T>
inline constexpr size_t findFirstSet(T x)
{
    return ffsl(x);
}

template <typename T>
inline constexpr size_t findLastSet(T x)
{
    return flsl(x);
}

template <typename T>
inline std::string toBitStr(T x)
{
    std::bitset<sizeof(T)*CHAR_BIT> bits(x);
    return bits.to_string();
}

} // namespace util 

#endif

