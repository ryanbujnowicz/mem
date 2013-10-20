#ifndef MEM_TRACKING_H
#define MEM_TRACKING_H

#include "util/stackTrace.h"
#include "mem/sourceInfo.h"
#include "mem/mallocAllocator.h"

namespace mem {

struct TrackingInfo
{
    void* mem;
    size_t size;
    size_t alignment;
    char* filename;
    size_t lineNumber;

    // Call stacks may be null
    int callstackSize;
    char** callstack;

    TrackingInfo* prev;
    TrackingInfo* next;
};

/**
 * Fulfills TrackingPolicy.
 */
class NoTracking
{
public:
    inline void onAllocation(void* mem, size_t size, size_t alignment, SourceInfo sourceInfo) const { }
    inline void onRelease(void* mem) const { }
};

class CountTracking
{
public:
    CountTracking() :
        _count(0)
    {
    }

    inline void onAllocation(void* mem, size_t size, size_t alignment, SourceInfo sourceInfo)
    {
        _count++;
    }

    inline void onRelease(void* mem)
    {
        assert(_count > 0 && "More release calls than allocations.");
        _count--;
    }

    inline int getNumberOfAllocations() const
    {
        return _count;
    }

private:
    int _count;
};

class SourceTracking
{
public:
    SourceTracking() :
        _allocList(nullptr)
    {
    }

    void onAllocation(void* mem, size_t size, size_t alignment, SourceInfo sourceInfo)
    {
        size_t len = strlen(sourceInfo.filename.c_str());
        char* filename = (char*)_allocator.allocate(len + 1, 1, 0);
        strncpy(filename, sourceInfo.filename.c_str(), len + 1);

        TrackingInfo* entry = (TrackingInfo*)_allocator.allocate(sizeof(TrackingInfo), 1, 0);
        entry->mem = mem;
        entry->size = size;
        entry->alignment = alignment;
        entry->filename = filename;
        entry->lineNumber = sourceInfo.lineNumber;
        entry->callstackSize = 0;
        entry->callstack = nullptr;
        entry->next = nullptr;

        if (!_allocList) {
            entry->prev = nullptr;
            _allocList = entry;
            return;
        }

        TrackingInfo* ptr = _allocList;
        while (ptr) {
            if (!ptr->next) {
                break;
            }
            ptr = ptr->next;
        }

        ptr->next = entry;
        entry->prev = ptr; 
    }

    void onRelease(void* mem)
    {
        TrackingInfo* ptr = _allocList; 
        while (ptr) {
            if (ptr->mem == mem) {
                if (ptr->prev) {
                    ptr->prev->next = ptr->next;
                }
                if (ptr->next) {
                    ptr->next->prev = ptr->prev;
                }
                if (ptr == _allocList) {
                    _allocList = ptr->next;
                }

                // TODO: stash these away for re-use
                _allocator.release(ptr->filename);
                _allocator.release(ptr);
                return;
            }
            ptr = ptr->next;
        }
        assert(false && "onRelease called before onAllocation");
    }

    TrackingInfo* getAllocations() const
    {
        return _allocList;
    }

private: 
    // TODO: Replace with a global 'overhead' allocator
    mem::MallocAllocator _allocator;
    TrackingInfo* _allocList;
};

class CallStackTracking
{
public:
    CallStackTracking() :
        _allocList(nullptr)
    {
    }

    void onAllocation(void* mem, size_t size, size_t alignment, SourceInfo sourceInfo)
    {
        size_t len = strlen(sourceInfo.filename.c_str());
        char* filename = (char*)_allocator.allocate(len + 1, 8, 0);
        strncpy(filename, sourceInfo.filename.c_str(), len + 1);

        // Skip the getStackTrace and this call from the stack trace
        int nStackEntries;
        char** tmpStackTrace = util::getStackTrace(&nStackEntries, 2);

        // These entries are allocated with malloc/free, copy them to some
        // memory allocated with our allocator for better tracking.
        char** stackTrace = (char**)_allocator.allocate(sizeof(char*)*nStackEntries, 8, 0);
        for (int i = 0; i < nStackEntries; ++i) {
            int len = strlen(tmpStackTrace[i]);
            stackTrace[i] = (char*)_allocator.allocate(len + 1, 8, 0);
            strncpy(stackTrace[i], tmpStackTrace[i], len + 1);
            free(tmpStackTrace[i]);
        }
        free(tmpStackTrace);

        TrackingInfo* entry = (TrackingInfo*)_allocator.allocate(sizeof(TrackingInfo), 1, 0);
        entry->mem = mem;
        entry->size = size;
        entry->alignment = alignment;
        entry->filename = filename;
        entry->lineNumber = sourceInfo.lineNumber;
        entry->callstackSize = nStackEntries;
        entry->callstack = stackTrace;
        entry->next = nullptr;

        if (!_allocList) {
            entry->prev = nullptr;
            _allocList = entry;
            return;
        }

        TrackingInfo* ptr = _allocList;
        while (ptr) {
            if (!ptr->next) {
                break;
            }
            ptr = ptr->next;
        }

        ptr->next = entry;
        entry->prev = ptr; 
    }

    void onRelease(void* mem)
    {
        TrackingInfo* ptr = _allocList; 
        while (ptr) {
            if (ptr->mem == mem) {
                if (ptr->prev) {
                    ptr->prev->next = ptr->next;
                }
                if (ptr->next) {
                    ptr->next->prev = ptr->prev;
                }
                if (ptr == _allocList) {
                    _allocList = ptr->next;
                }

                for (int i = 0; i < ptr->callstackSize; ++i) {
                    _allocator.release(ptr->callstack[i]);
                }
                _allocator.release(ptr->callstack);

                // TODO: stash these away for re-use
                _allocator.release(ptr->filename);
                _allocator.release(ptr);
                return;
            }
            ptr = ptr->next;
        }
        assert(false && "onRelease called before onAllocation");
    }

    TrackingInfo* getAllocations() const
    {
        return _allocList;
    }

private:
    // TODO: Replace with a global 'overhead' allocator
    mem::MallocAllocator _allocator;
    TrackingInfo* _allocList;
};

} // namespace mem

#endif

