#ifndef MEM_MARKING_H
#define MEM_MARKING_H

namespace mem {

/**
 * Fulfills the MarkingPolicy concept.
 */
class NoMarking
{
public:
    inline void onAllocation(void* mem, size_t size) const {}
    inline void onRelease(void* mem, size_t size) const {}
};

class Marking
{
public:
    inline void onAllocation(void* mem, size_t size) const 
    {
        char src[] = {0xC, 0xD};  
        size_t j = 0;

        char* memc = static_cast<char*>(mem);
        for (size_t i = 0; i < size; ++i) {
            memc[i] = src[j++];
            j %= 2;
        }
    }

    inline void onRelease(void* mem, size_t size) const 
    {
        char src = 0xD;
        char* memc = static_cast<char*>(mem);
        for (size_t i = 0; i < size; ++i) {
            memc[i] = src;
        }
    }
};

} // namespace mem

#endif

