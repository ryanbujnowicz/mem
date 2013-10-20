#ifndef MEM_LINEARALLOCATOR_H
#define MEM_LINEARALLOCATOR_H

#include "mem/alignment.h"
#include "mem/allocator.h"
#include "mem/util.h"

namespace mem {

/**
 * Fast allocator which doesn't allow for releasing of individual blocks.
 *
 * Allocations are very cheap and fast since no per-block metadata is kept. Because of this 
 * individual blocks can't be released, only the entire allocator can be cleared. This is most 
 * useful for transient memory data such as information which is allocated and then cleared per 
 * frame.
 */
class LinearAllocator : public mem::Allocator
{
public:
    struct Stats
    {
        size_t allocatedBytes;
        size_t freeBytes;
    };

public:
    LinearAllocator(void *mem, size_t size, size_t alignment = 4);
    ~LinearAllocator();

    virtual void* allocate(size_t size, size_t alignment = DefaultAlignment, size_t offset = 0) override;
    virtual void release(void* addr) override;
    virtual size_t getAllocationSize(void* addr) const override
    {
        // Not implemented
        return 0;
    }

    void clear();

    Stats getStats() const;

private:
    void* _startAddr;
    void* _endAddr;
    void* _curAddr;
    const size_t _alignment;
};

LinearAllocator::LinearAllocator(void* mem, size_t size, size_t alignment) :
    _startAddr(mem),
    _endAddr((char*)_startAddr + size),
    _curAddr(mem::align(_startAddr, alignment)),
    _alignment(alignment)
{
}

LinearAllocator::~LinearAllocator()
{
}

void* LinearAllocator::allocate(size_t numBytes, size_t, size_t)
{
    void* addr = _curAddr;
    if (static_cast<char*>(addr) + numBytes > static_cast<char*>(_endAddr)) {
        return nullptr;
    }

    _curAddr = mem::align(static_cast<char*>(_curAddr) + numBytes, _alignment);
    return addr; 
}

void LinearAllocator::release(void* addr)
{
    // Noop
}

void LinearAllocator::clear() 
{
    _curAddr = _startAddr;
}

LinearAllocator::Stats LinearAllocator::getStats() const
{
    LinearAllocator::Stats stats;
    stats.allocatedBytes = (size_t)_curAddr - (size_t)_startAddr;
    stats.freeBytes = (size_t)_endAddr - (size_t)_curAddr;
    return stats;
}

} // namespace mem

#endif

