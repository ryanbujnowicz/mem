#ifndef MEM_NEW_H
#define MEM_NEW_H

#include <type_traits>

#include "mem/alignment.h"
#include "mem/sourceInfo.h"
#include "util/typeTraits.h"

/**
 * Macros which wrap the memory allocation/deallocation functions.
 */
#define _MEM_NEW_IMPL(type, region) \
    new (region, __FILE__, __LINE__) type

#define _MEM_NEW_ALIGN_IMPL(type, alignment, region) \
    new (region, alignment, __FILE__, __LINE__) type

#define _MEM_DELETE_IMPL(obj, region) \
    mem::deleteMem(obj, region)

#define _MEM_NEW_ARRAY_IMPL(type, region) \
    mem::newArray<util::BaseType<type>::Type>(region, util::BaseType<type>::Count, __FILE__, __LINE__)

#define _MEM_DELETE_ARRAY_IMPL(obj, region) \
    mem::deleteArray(obj, region)

#define MEM_NEW(type, regionId) \
    _MEM_NEW_IMPL(type, mem::getRegion(regionId))

#define MEM_NEW_ALIGN(type, alignment, regionId) \
    _MEM_NEW_ALIGN_IMPL(type, alignment, mem::getRegion(regionId))

#define MEM_DELETE(obj, regionId) \
    _MEM_DELETE_IMPL(obj, mem::getRegion(regionId))

#define MEM_NEW_ARRAY(type, regionId) \
    _MEM_NEW_ARRAY_IMPL(type, mem::getRegion(regionId))

#define MEM_DELETE_ARRAY(obj, regionId) \
    _MEM_DELETE_ARRAY_IMPL(obj, mem::getRegion(regionId))

namespace 
{

typedef std::true_type PodType;
typedef std::false_type NonPodType;

template <class T, class Region>
void deleteMem(T* object, Region& region, PodType)
{
    region.release(object);
}

template <class T, class Region>
void deleteMem(T* object, Region& region, NonPodType)
{
    object->~T();
    region.release(object);
}

template <class T, class Region>
T* newArray(Region& region, size_t n, const char* file, int line, PodType)
{
    return static_cast<T*>(region.allocate(n*sizeof(T), mem::DefaultAlignment, 
                mem::SourceInfo(file, line)));
}

template <class T, class Region>
T* newArray(Region& region, size_t n, const char* file, int line, NonPodType)
{
    union 
    {
        void* asVoid;
        size_t* asSizeT;
        T* asT;
    };

    // Allocate an extra sizeof(size_t) to store the number of elements in this arrya at
    // the front.
    asVoid = region.allocate(sizeof(size_t) + n*sizeof(T), mem::DefaultAlignment, 
            mem::SourceInfo(file, line));

    *asSizeT++ = n;

    // Construct each of the array elements using placement new on the allocated memory
    const T* end = asT + n;
    while (asT != end) {
        new (asT++) T;
    }

    // Don't include the size header in the returned pointer
    return asT - n;
}

template <class T, class Region>
void deleteArray(T* ptr, Region& region, PodType)
{
    region.release(ptr);
}

template <class T, class Region>
void deleteArray(T* ptr, Region& region, NonPodType)
{
    union 
    {
        size_t* asSizeT;
        T* asT;
    };

    asT = ptr;

    // ptr points to the first index of the array, the size is a hidden field before that
    const size_t n = asSizeT[-1];

    // Deconstruct in reverse order of how we constructed
    for (size_t i = n; i > 0; --i) {
        asT[i - 1].~T();
    }

    region.release(asSizeT - 1);
}

}

/**
 * new has to exist outside of the mem namespace in order for us to use the placement new syntax. 
 */
template <class Region>
void* operator new(size_t bytes, Region& region, const char* file, int line)
{
    return region.allocate(bytes, mem::DefaultAlignment, mem::SourceInfo(file, line));
}

template <class Region>
void* operator new(size_t bytes, Region& region, size_t alignment, const char* file, int line)
{
    return region.allocate(bytes, alignment, mem::SourceInfo(file, line));
}

namespace mem
{

template <class T, class Region>
void deleteMem(T* object, Region& region)
{
    ::deleteMem(object, region, std::is_pod<T>());
}

template <class T, class Region>
T* newArray(Region& region, size_t n, const char* file, int line)
{
    return ::newArray<T>(region, n, file, line, std::is_pod<T>());
}

template <class T, class Region>
void deleteArray(T* ptr, Region& region)
{
    return ::deleteArray(ptr, region, std::is_pod<T>());
}

} 
 
#endif

