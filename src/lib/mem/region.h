#ifndef MEM_REGION_H
#define MEM_REGION_H

#include "mem/sourceInfo.h"

namespace mem {

/**
 * This provides a base class so we can polymorphically use the Regions regardless
 * of their specific policies.
 */
class RegionBase
{
public:
    virtual ~RegionBase() {};
    virtual void* allocate(size_t size, size_t alignment, SourceInfo sourceInfo) = 0;
    virtual void release(void* mem) = 0;
};

/**
 * A Region defines a series of policies defining how memory is to be allocated.
 * 
 * The idea for this was heavily inspired by the blog MolecularMusings.
 */
template <
    class AllocationPolicy, 
    class ThreadingPolicy, 
    class BoundsCheckingPolicy, 
    class TrackingPolicy, 
    class MarkingPolicy
    >
class Region : public RegionBase
{
public:
    template <class AreaPolicy>
    explicit Region(const AreaPolicy& areaPolicy) :
        _allocator(areaPolicy.start(), areaPolicy.end())
    {

    }

    explicit Region(size_t size) :
        _allocator(size)
    {

    }

    explicit Region() :
        _allocator()
    {

    }

    TrackingPolicy trackingPolicy() const 
    {
        return _tracker;
    }

    void* allocate(size_t size, size_t alignment, SourceInfo sourceInfo)
    {
        _threadGuard.begin();

        const size_t originalSize = size;
        const size_t newSize = size + BoundsCheckingPolicy::SizeFront + BoundsCheckingPolicy::SizeBack;

        void* mem = _allocator.allocate(newSize, alignment, BoundsCheckingPolicy::SizeFront);
        char* memc = static_cast<char*>(mem);

        _boundsChecker.guardFront(memc);
        _boundsChecker.guardBack(memc + BoundsCheckingPolicy::SizeFront + originalSize);
        _marker.onAllocation(memc + BoundsCheckingPolicy::SizeFront, originalSize);
        _tracker.onAllocation(memc, newSize, alignment, sourceInfo);

        _threadGuard.end();
        return memc + BoundsCheckingPolicy::SizeFront;
    }

    void release(void* addr)
    {
        _threadGuard.begin();

        char* origMem = (char*)addr - BoundsCheckingPolicy::SizeFront;
        const size_t allocSize = _allocator.getAllocationSize(addr);

        _boundsChecker.checkFront(origMem);
        _boundsChecker.checkBack(origMem + BoundsCheckingPolicy::SizeFront + allocSize);

        _tracker.onRelease(origMem);
        _marker.onRelease(origMem, allocSize);

        _allocator.release(addr);

        _threadGuard.end();
    }

protected:
    AllocationPolicy _allocator;
    ThreadingPolicy _threadGuard;
    BoundsCheckingPolicy _boundsChecker;
    TrackingPolicy _tracker;
    MarkingPolicy _marker;
};

/**
 * Public API for the region registry.
 */

static const int DefaultRegion = -1;
extern void registerRegion(RegionBase& region, const int id);
extern RegionBase& getRegion(int id);
extern void setDefaultRegion(int id);

} // namespace mem

#endif

