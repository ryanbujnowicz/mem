#ifndef UTIL_ALIGN_H
#define UTIL_ALIGN_H

#include "util/math.h"

namespace util {

/**
 * Determines the next power-of-two aligned address.
 */
void* align(void* addr, size_t alignment)
{
    size_t saddr = (size_t)addr;
    return (void*)(util::nextPowerOfTwoMultiple(saddr, alignment));
}

}

#endif

