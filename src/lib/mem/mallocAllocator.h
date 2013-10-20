#ifndef MEM_MALLOCALLOCATOR_H
#define MEM_MALLOCALLOCATOR_H

#include "mem/alignment.h"
#include "mem/allocator.h"

namespace mem {

/**
 * A small wrapper around the sytem malloc/free.
 *
 * Additional features include allowing for determining the size of a previous allocation
 * and support for differeing alignments. Both of these features do add to the memory
 * footprint of the allocator.
 *
 * Implemented AllocatorPolicy.
 */
class MallocAllocator : public Allocator
{
private:
    typedef size_t SizeField;
    typedef char AlignOffsetField;

public:
    explicit MallocAllocator()
    {
    }
    
    ~MallocAllocator() { }

    /**
     * _offset_ is the amount of bytes to leave empty at the start of the allocated memory
     * (before even the internal memory bookkeeping). The offset bytes are already included
     * in the passed in size value. This is used by the region system to implement features 
     * like bounds checking bytes.
     *
     * The memory map for how the result from memory is sliced up is:
     *
     *                 +------------------+
     *     malloc() -> | alignOffset      |
     *                 +------------------+
     *                 | AlignOffsetField |
     *                 +------------------+
     *                 | SizeField        |
     *                 +------------------+
     *                 | offset           | -> allocate()
     *                 +------------------+
     *                 | Aligned memory   |
     *                 +------------------+
     */
    virtual void* allocate(size_t size, size_t alignment = DefaultAlignment, size_t offset = 0) override
    {
        // offset is already included in size
        const size_t newSize = size + (alignment - 1) + sizeof(SizeField) + sizeof(AlignOffsetField);
        char* alloc = static_cast<char*>(malloc(newSize));

        char* preAlignedMem = alloc + sizeof(AlignOffsetField) + sizeof(SizeField) + offset;
        // TODO: align func
        size_t alignOffset = (alignment - ((size_t)(preAlignedMem)%alignment))%alignment;
        char* alignedMem = preAlignedMem + alignOffset;
        char* retMem = alignedMem - offset;
        
        SizeField* sizeMem = (SizeField*)(retMem - sizeof(SizeField));
        *sizeMem = newSize;

        char* alignOffsetMem = (char*)sizeMem - sizeof(AlignOffsetField);
        *alignOffsetMem = static_cast<char>(alignOffset);

        return static_cast<void*>(retMem);
    }

    virtual void release(void* mem) override
    {
        char* alignOffsetMem = static_cast<char*>(mem) - sizeof(SizeField) - sizeof(AlignOffsetField);
        size_t alignOffset = *alignOffsetMem;

        char* originalMem = alignOffsetMem - alignOffset;
        free(originalMem);
    }

    virtual size_t getAllocationSize(void* mem) const override
    {
        char* sizeMem = static_cast<char*>(mem) - sizeof(SizeField);
        return *(size_t*)(sizeMem);
    }
};

} // namespace mem

#endif

