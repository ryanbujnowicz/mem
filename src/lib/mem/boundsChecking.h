#ifndef MEM_BOUNDSCHECKING_H
#define MEM_BOUNDSCHECKING_H

#include <cassert>

namespace mem {

/**
 * Fulfills the BoundsCheckingPolicy concept.
 */
class NoBoundsChecking
{
public:
    static const size_t SizeFront = 0;
    static const size_t SizeBack = 0;

    inline void guardFront(void* mem) const { }
    inline void guardBack(void* mem) const { }
    inline bool checkFront(void* mem) const { return true; } 
    inline bool checkBack(void* mem) const { return true; }
};

/**
 * Inserts a specific byte sequence at the start and end of the memory region to help
 * check for bounds overruns.
 *
 * Fulfills the BoundsCheckingPolicy concept.
 */
class BoundsChecking
{
private:
    static const int FrontSequence = 0x01234567;
    static const int BackSequence = 0x89ABCDEF;

public:
    static const size_t SizeFront = sizeof(FrontSequence);
    static const size_t SizeBack = sizeof(BackSequence);

    inline void guardFront(void* mem)
    { 
        int* intMem = static_cast<int*>(mem); 
        *intMem = FrontSequence;
    }

    inline void guardBack(void* mem)
    { 
        int* intMem = static_cast<int*>(mem); 
        *intMem = BackSequence;
    }

    inline bool checkFront(void* mem)
    { 
        int* intMem = static_cast<int*>(mem); 

        if (*intMem != FrontSequence) {
            //assert(false && "Front bounds check failed");
            return false;
        }
        return true;
    }

    inline bool checkBack(void* mem)
    { 
        int* intMem = static_cast<int*>(mem); 

        if (*intMem != BackSequence) {
            //assert(false && "Back bounds check failed");
            return false;
        }
        return true;
    }
};

// TODO: a way to check every allocation

} // namespace mem

#endif

