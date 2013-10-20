#ifndef UTIL_MEMORY_H
#define UTIL_MEMORY_H

#include <cassert>
#include <sys/mman.h>
#include <unistd.h>

namespace util {

inline size_t getPageSize()
{
    return getpagesize();
}

/**
 * Allocates one or more pages totalling the given size from the OS.
 *
 * _size_ must be a multiple of the OS page size as determined by getPageSize(). The result of this
 * call should be deallocated by calling the pageRelease function.
 */
inline void* pageAllocate(size_t size)
{
    assert(size > 0 && size%getPageSize() == 0);
    void* mem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    assert(mem != MAP_FAILED);
    return mem;
}

inline void pageRelease(void* mem, size_t size)
{
    assert(mem);
    int err = munmap(mem, size);
    assert(err == 0);
    (void)err;
}

} // namespace util 

#endif

