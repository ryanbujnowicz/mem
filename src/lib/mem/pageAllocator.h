#ifndef MEM_LINEARALLOCATOR_H
#define MEM_LINEARALLOCATOR_H

#include "mem/alignment.h"
#include "mem/allocator.h"

namespace mem {

/**
 * Allocates pages of memory from the operating system.
 *
 * Fulfills the AllocatorPolicy concept.
 * TODO: noncopyable
 */
class PageAllocator : public mem::Allocator
{
public:
    PageAllocator();
    ~PageAllocator();

    /**
     * _allocate_ returns a number of bytes a multiple of the system page size which in practice is
     * usually 4096 bytes.
     */
    virtual void* allocate(size_t size, size_t alignment, size_t offset = 0) override;
    virtual void release(void* addr) override;

    virtual size_t getAllocationSize(void* mem) const override;

protected:
    struct Segment
    {
        Segment* next; 
        Segment* prev; 
        size_t size;
        size_t alignOffset;
        size_t offset;
    };

    void _linkSegment(Segment* segment);
    void _unlinkSegment(Segment* segment);
    void _releaseSegment(Segment* segment);

    Segment* _getSegmentFromMem(void* mem) const;
    void* _getMemFromSegment(Segment* segment) const;
    size_t _getPageSize(Segment* segment) const;

private:
    Segment* _segmentList;
};

} // namespace mem

#endif

