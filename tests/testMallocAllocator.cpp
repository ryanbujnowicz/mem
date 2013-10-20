#include <gtest/gtest.h>

#include "mem/mallocAllocator.h"
#include "util/units.h"

TEST(MallocAllocator, ZeroSizeAlloc)
{
    mem::MallocAllocator alloc;
    void* x = alloc.allocate(0);
    EXPECT_TRUE(x != nullptr);
}

TEST(MallocAllocator, Stress)
{
    // This test just does a ton of random memory 'events' aka
    // allocations and de-allocations. Use a consistent seed to
    // make this repeatable and debuggable.
    mem::MallocAllocator alloc;
    std::vector<void*> allocs;

    const size_t RandSeed = 121;
    srand(RandSeed);

    const double AllocChance = 0.50;

    const size_t NumEvents = 1000000;
    for (size_t i = 0; i < NumEvents; ++i) {
        // Determine if we are allocating or freeing
        if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
            size_t numBytes = rand()%util::kilobytes(8);
            void* x = alloc.allocate(numBytes);
            allocs.push_back(x);
        } else {
            size_t releaseIndex = rand()%allocs.size();
            alloc.release(allocs[releaseIndex]);
            allocs.erase(allocs.begin() + releaseIndex);
        }
    }
    
    for (void* ptr: allocs) {
        alloc.release(ptr);
    }
}

TEST(MallocAllocator, Alignment)
{
    const size_t NumEvents = 10000;
    std::vector<void*> allocs;

    const float AllocChance = 0.7;

    const size_t alignments[] = {1,2,4,8,16};

    // Best way to test is just to do a bunch of allocations/deletions
    // and ensure the alignment is right. 
    {
        mem::MallocAllocator alloc;

        for (size_t i = 0; i < NumEvents; ++i) {
            // Determine if we are allocating or freeing
            if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
                size_t numBytes = rand()%util::kilobytes(8);
                size_t align = alignments[rand()%5];

                void* x = alloc.allocate(numBytes, align);
                allocs.push_back(x);
                EXPECT_TRUE((size_t)x%align == 0);
            } else {
                size_t releaseIndex = rand()%allocs.size();
                alloc.release(allocs[releaseIndex]);
                allocs.erase(allocs.begin() + releaseIndex);
            }
        }
        
        // Free any remaining
        for (void* ptr: allocs) {
            alloc.release(ptr);
        }
        allocs.clear();
    }
}

TEST(MallocAllocator, GetAllocationSize)
{
    // This comes from the MallocAllocator implementation as is fixed when the alignment is 1
    const size_t Overhead = sizeof(char) + sizeof(size_t);

    mem::MallocAllocator alloc;
    void* x = alloc.allocate(12, 1);
    EXPECT_EQ(12 + Overhead, alloc.getAllocationSize(x));

    x = alloc.allocate(13, 1);
    EXPECT_EQ(13 + Overhead, alloc.getAllocationSize(x));

    x = alloc.allocate(106, 1);
    EXPECT_EQ(106 + Overhead, alloc.getAllocationSize(x));

    // With alignment we can't be exactly sure
    x = alloc.allocate(106, 4);
    size_t diff = alloc.getAllocationSize(x) - 106;
    EXPECT_LE(Overhead + 3, diff);
}

