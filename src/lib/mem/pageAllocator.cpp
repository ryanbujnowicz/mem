#include "mem/pageAllocator.h"
using namespace mem;

#include "util/align.h"
#include "util/math.h"
#include "util/memory.h"

PageAllocator::PageAllocator() :
    _segmentList(nullptr)
{
}

PageAllocator::~PageAllocator()
{
    Segment* ptr = _segmentList;
    while (ptr) {
        Segment* next = ptr->next;
        _releaseSegment(ptr);
        ptr = next;
    }
}

/**
 * The memory map for how the result from memory is sliced up is:
 *
 *                    +-------------------------+
 *  pageAllocate() -> | alignOffset             |
 *                    +-------------------------+
 *                    | Segment                 |
 *                    +-------------------------+
 *                    | offset                  | -> allocate()
 *                    +-------------------------+
 *                    | Aligned memory          | 
 *                    +-------------------------+
 */
void* PageAllocator::allocate(size_t size, size_t alignment, size_t offset)
{
    // The offset should already be accounted for in the size
    size_t allocSize = size + (alignment - 1) + sizeof(Segment);
    // TODO: alignment function
    size_t pageAlignedSize = util::nextPowerOfTwoMultiple(allocSize, util::getPageSize());
    char* allocMem = (char*)util::pageAllocate(pageAlignedSize);

    // TODO: alignment function
    char* preAlignedMem = (char*)allocMem + sizeof(Segment) + offset;
    size_t alignOffset = (alignment - ((size_t)preAlignedMem%alignment))%alignment;
    assert((size_t)(preAlignedMem + alignOffset)%alignment == 0);

    Segment* segment = (Segment*)(allocMem + alignOffset);
    segment->size = pageAlignedSize - offset - alignOffset - sizeof(Segment);
    segment->alignOffset = alignOffset;
    _linkSegment(segment);

    return _getMemFromSegment(segment);
}

void PageAllocator::release(void* mem)
{
    assert(mem);

    Segment* segment = _getSegmentFromMem(mem);
    _unlinkSegment(segment);
    _releaseSegment(segment);
}

size_t PageAllocator::getAllocationSize(void* mem) const
{
    assert(mem);
    Segment* segment = _getSegmentFromMem(mem);
    return segment->size;
}

void PageAllocator::_linkSegment(Segment* segment)
{
    assert(segment);
    if (!_segmentList) {
        segment->next = nullptr;
        segment->prev = nullptr;
        _segmentList = segment;
        return;
    }

    Segment* ptr = _segmentList;
    while (ptr) {
        Segment* nextPtr = ptr->next;
        if (!nextPtr) {
            break;
        }
        ptr = nextPtr;
    }

    Segment* lastSegment = ptr;
    lastSegment->next = segment;
    segment->next = nullptr;
    segment->prev = lastSegment;
}

void PageAllocator::_unlinkSegment(Segment* segment)
{
    assert(segment);
    if (segment == _segmentList) {
        _segmentList = segment->next;
    }
    if (segment->next) {
        segment->next->prev = segment->prev;
    }
    if (segment->prev) {
        segment->prev->next = segment->next;
    }
}

void PageAllocator::_releaseSegment(Segment* segment)
{
    assert(segment);
    void* pageMem = (char*)(segment) - segment->alignOffset;
    util::pageRelease(pageMem, _getPageSize(segment));
}

PageAllocator::Segment* PageAllocator::_getSegmentFromMem(void* mem) const
{
    assert(mem);
    return (PageAllocator::Segment*)(static_cast<char*>(mem) - sizeof(Segment));
}

void* PageAllocator::_getMemFromSegment(Segment* segment) const
{
    assert(segment);
    return (char*)(segment) + sizeof(Segment);
}

size_t PageAllocator::_getPageSize(Segment* segment) const
{
    assert(segment);
    return segment->size + segment->alignOffset + segment->offset + sizeof(Segment);
}

