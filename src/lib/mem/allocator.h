#ifndef MEM_ALLOCATOR_H
#define MEM_ALLOCATOR_H

#include <cstdlib>

namespace mem {

/**
 * Defines the public interface for a mem library Allocator. 
 *
 * This is mostly necessary when working with allocators in a generic way, such as when
 * interacting with STL containers. We do take a slight performance hit doing the virtual
 * table lookup in such cases.
 */
class Allocator
{
public:
    virtual ~Allocator() {};
    virtual void* allocate(size_t size, size_t alignment = DefaultAlignment, size_t offset = 0) = 0;
    virtual void release(void* addr) = 0;
    virtual size_t getAllocationSize(void* mem) const = 0;
};

} 

#endif

