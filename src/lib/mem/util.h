#ifndef MEM_UTIL_H
#define MEM_UTIL_H

#include "util/math.h"

namespace mem {

/**
 * Determines the next address from the given one which has the given alignment.
 *
 * alignment must be a non-zero power-of-2.
 */
inline void* align(void* addr, size_t alignment)
{
    assert(util::isPowerOfTwo(alignment) && "Power of two alignment required");
    return (void*)(
            (char*)(addr) + alignment - 1 - 
            (uintptr_t)((char*)(addr) - 1)%alignment);
}

inline size_t align(size_t n, size_t alignment)
{
    assert(util::isPowerOfTwo(alignment) && "Power of two alignment required");
    return n + alignment - 1 - 
           ((n - 1)%alignment);
}

} // namespace mem

#endif

