#include <gtest/gtest.h>

#include "mem/pageAllocator.h"
#include "util/units.h"

TEST(PageAllocator, ZeroSizeAlloc)
{
    mem::PageAllocator alloc;
    void* x = alloc.allocate(0, 4);
    EXPECT_TRUE(x != nullptr);
}

TEST(PageAllocator, Stress)
{
    // This test just does a ton of random memory 'events' aka
    // allocations and de-allocations. Use a consistent seed to
    // make this repeatable and debuggable.
    mem::PageAllocator alloc;
    std::vector<void*> allocs;

    const size_t RandSeed = 121;
    srand(RandSeed);

    const double AllocChance = 0.20;

    const size_t NumEvents = 1000;
    for (size_t i = 0; i < NumEvents; ++i) {
        // Determine if we are allocating or freeing
        if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
            size_t numBytes = rand()%util::kilobytes(8);
            void* x = alloc.allocate(numBytes, 4);
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

TEST(PageAllocator, Alignment)
{
    const size_t NumEvents = 10000;
    std::vector<void*> allocs;

    const size_t RandSeed = 69;
    srand(RandSeed);

    const float AllocChance = 0.7;

    const size_t alignments[] = {1,2,4,8,16};

    // Best way to test is just to do a bunch of allocations/deletions
    // and ensure the alignment is right. 
    {
        mem::PageAllocator alloc;

        for (size_t i = 0; i < NumEvents; ++i) {
            // Determine if we are allocating or freeing
            if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
                size_t numBytes = rand()%util::kilobytes(8);
                size_t align = alignments[rand()%5];

                void* x = alloc.allocate(numBytes, align);
                allocs.push_back(x);
                EXPECT_TRUE(((size_t)x)%align == 0);
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

TEST(PageAllocator, Offset)
{
    const size_t NumEvents = 10000;
    std::vector<void*> allocs;

    const size_t RandSeed = 1001;
    srand(RandSeed);

    const float AllocChance = 0.7;

    const size_t alignments[] = {1,2,4,8,16};

    // Best way to test is just to do a bunch of allocations/deletions
    // and ensure the alignment is right. 
    {
        mem::PageAllocator alloc;

        for (size_t i = 0; i < NumEvents; ++i) {
            // Determine if we are allocating or freeing
            if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
                size_t numBytes = rand()%util::kilobytes(8);
                size_t align = alignments[rand()%5];
                size_t offset = rand()%16;

                void* x = alloc.allocate(numBytes, align, offset);
                allocs.push_back(x);
                //EXPECT_TRUE(((size_t)x + offset)%align == 0);
                EXPECT_TRUE(((size_t)x + offset)%align == 0);

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

TEST(PageAllocator, GetAllocationSize)
{
    mem::PageAllocator alloc;
    void* x = alloc.allocate(12, 1);
    EXPECT_EQ(4056, alloc.getAllocationSize(x));

    x = alloc.allocate(13, 1);
    EXPECT_EQ(4056, alloc.getAllocationSize(x));

    x = alloc.allocate(106, 1);
    EXPECT_EQ(4056, alloc.getAllocationSize(x));

    x = alloc.allocate(5000, 1);
    EXPECT_EQ(8152, alloc.getAllocationSize(x));
}

