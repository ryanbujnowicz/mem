#ifndef MEM_STL_ADAPTER_H
#define MEM_STL_ADAPTER_H

#include <limits>

#include "mem/alignment.h"
#include "mem/region.h"

namespace mem {

/**
 * Provides the necessary interface between the memory regions and the STL.
 *
 * This is a thin wrapper which allows our custom memory allocators to be used by STL containers
 * like vector, map, string, etc. 
 */
template <class T>
class StlAdapter
{
public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

public:
    template <class U>
    struct rebind
    {
        typedef StlAdapter<U> other;
    };

    pointer address(reference value) const
    {
        return &value;
    }

    const_pointer address(const_reference value) const
    {
        return &value;
    }

    StlAdapter(mem::RegionBase& region) :
        _region(region)
    {
    }

    StlAdapter(const StlAdapter& rhs) :
        _region(rhs._region)
    {
    }

    // assignment operator?

    template <class U>
    StlAdapter(const StlAdapter<U>& rhs) :
        _region(rhs.region())
    {
    }

    size_type max_size() const
    {
        return std::numeric_limits<std::size_t>::max() / sizeof(T);
    }

    pointer allocate(size_type num, const void* = 0)
    {
        pointer ret = (pointer)_region.allocate(num*sizeof(T), mem::DefaultAlignment, mem::SourceInfo("stl_internal", 0));
        return ret;
    }

    void construct(pointer obj, const T& value)
    {
        new (obj) T(value);
    }

    void destroy(pointer obj, const T& value)
    {
        obj->~T();
    }

    void deallocate(pointer obj, size_type num)
    {
        _region.release(obj);
    }

    mem::RegionBase& region() const
    {
        return _region;
    }

private:
    // Must be called with an allocator argument
    StlAdapter();

    mem::RegionBase& _region;
};

// This allocator is equivalent to this allocator of any specialization
template <class T1, class T2>
bool operator==(const StlAdapter<T1>&, const StlAdapter<T2>&)
{
    return true;
}

template <class T1, class T2>
bool operator!=(const StlAdapter<T1>&, const StlAdapter<T2>&)
{
    return false;
}

} // namespace mem

#endif

