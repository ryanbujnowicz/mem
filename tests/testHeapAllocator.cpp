#include <cstdlib>

#include <gtest/gtest.h>
#include <string>

#include "mem/heapAllocator.h"

#include "util/unused.h"
#include "util/stopwatch.h"

TEST(HeapAllocator, ZeroSizeAlloc)
{
    mem::HeapAllocator allocator;
    void* x = allocator.allocate(0);
    UNUSED(x);
    EXPECT_TRUE(x != nullptr);
    EXPECT_TRUE(allocator.check());
}

TEST(HeapAllocator, BufferOverflowCheck)
{
    mem::HeapAllocator allocator;

    char* x = (char*)allocator.allocate(16);
    x[0] = 5;

    // Buffer overflow!
    EXPECT_TRUE(allocator.check());
    x[16] = 0;
    x[15] = 0;
    x[17] = 0;
    EXPECT_FALSE(allocator.check());
}

TEST(HeapAllocator, NewSegments)
{
    // Minimum initial size is a page size
    mem::HeapAllocator allocator(1024);

    // Segments will attempt to merge if possible, but whether
    // that happens depends on how to OS allocates mmap
    // regions. We can't rely on it happening everytime so
    // just disable merging for now.
    allocator.enableSegmentMerging(false);

    EXPECT_EQ(1, allocator.getStats().numRegularSegments);
    allocator.allocate(512);
    EXPECT_EQ(1, allocator.getStats().numRegularSegments);

    allocator.allocate(5000);
    EXPECT_EQ(2, allocator.getStats().numRegularSegments);

    // Make an external segment
    allocator.allocate(util::megabytes(20));
    EXPECT_EQ(3, allocator.getStats().numRegularSegments);

    allocator.allocate(10000);
    EXPECT_EQ(4, allocator.getStats().numRegularSegments);
}

TEST(DISABLED_HeapAllocator, OutOfMemory)
{
    // OOM will throw an assert in debug mode
    mem::HeapAllocator allocator(1000);
    allocator.enableSystemAllocation(false);

    void* x = allocator.allocate(1000);
    EXPECT_TRUE(x != nullptr);
    EXPECT_TRUE(allocator.check());

    x = allocator.allocate(4096);
    EXPECT_TRUE(x == nullptr);
    EXPECT_TRUE(allocator.check());

    allocator.release(x);
    EXPECT_TRUE(allocator.check());

    x = allocator.allocate(1000);
    EXPECT_TRUE(x != nullptr);
    EXPECT_TRUE(allocator.check());
}

TEST(HeapAllocator, GetStats)
{
    mem::HeapAllocator allocator;
    mem::HeapAllocator::Stats stats;
   
    stats = allocator.getStats();

    EXPECT_EQ(0, stats.allocatedBytes);
    EXPECT_EQ(69544, stats.freeBytes);
    EXPECT_EQ(56, stats.overheadBytes);
    EXPECT_EQ(0, stats.allocatedBlocks);
    EXPECT_EQ(1, stats.freeBlocks);
    EXPECT_EQ(1, stats.numRegularSegments);
    EXPECT_EQ(0, stats.numExternalSegments);

    void* x = allocator.allocate(1024);
    stats = allocator.getStats();

    EXPECT_EQ(1024, stats.allocatedBytes);
    EXPECT_EQ(68504, stats.freeBytes);
    EXPECT_EQ(72, stats.overheadBytes);
    EXPECT_EQ(1, stats.allocatedBlocks);
    EXPECT_EQ(1, stats.freeBlocks);
    EXPECT_EQ(1, stats.numRegularSegments);
    EXPECT_EQ(0, stats.numExternalSegments);

    void* y = allocator.allocate(1024);
    stats = allocator.getStats();

    EXPECT_EQ(2048, stats.allocatedBytes);
    EXPECT_EQ(67464, stats.freeBytes);
    EXPECT_EQ(88, stats.overheadBytes);
    EXPECT_EQ(2, stats.allocatedBlocks);
    EXPECT_EQ(1, stats.freeBlocks);
    EXPECT_EQ(1, stats.numRegularSegments);
    EXPECT_EQ(0, stats.numExternalSegments);

    void* z = allocator.allocate(100);
    stats = allocator.getStats();

    EXPECT_EQ(2148, stats.allocatedBytes);
    EXPECT_EQ(67348, stats.freeBytes);
    EXPECT_EQ(104, stats.overheadBytes);
    EXPECT_EQ(3, stats.allocatedBlocks);
    EXPECT_EQ(1, stats.freeBlocks);
    EXPECT_EQ(1, stats.numRegularSegments);
    EXPECT_EQ(0, stats.numExternalSegments);

    allocator.release(x);
    stats = allocator.getStats();

    EXPECT_EQ(1124, stats.allocatedBytes);
    EXPECT_EQ(68372, stats.freeBytes);
    EXPECT_EQ(104, stats.overheadBytes);
    EXPECT_EQ(2, stats.allocatedBlocks);
    EXPECT_EQ(2, stats.freeBlocks);
    EXPECT_EQ(1, stats.numRegularSegments);
    EXPECT_EQ(0, stats.numExternalSegments);

    // This will merge the two 1024 blocks together
    allocator.release(z);
    stats = allocator.getStats();

    EXPECT_EQ(1024, stats.allocatedBytes);
    EXPECT_EQ(68472, stats.freeBytes);
    EXPECT_EQ(104, stats.overheadBytes);
    EXPECT_EQ(1, stats.allocatedBlocks);
    EXPECT_EQ(3, stats.freeBlocks);
    EXPECT_EQ(1, stats.numRegularSegments);
    EXPECT_EQ(0, stats.numExternalSegments);

    allocator.release(y);
    stats = allocator.getStats();

    EXPECT_EQ(0, stats.allocatedBytes);
    EXPECT_EQ(69528, stats.freeBytes);
    EXPECT_EQ(72, stats.overheadBytes);
    EXPECT_EQ(0, stats.allocatedBlocks);
    EXPECT_EQ(2, stats.freeBlocks);
    EXPECT_EQ(1, stats.numRegularSegments);
    EXPECT_EQ(0, stats.numExternalSegments);
}

TEST(HeapAllocator, GetBlocks)
{
    mem::HeapAllocator allocator;
    std::vector<mem::HeapAllocator::Block> blocks;
   
    blocks = allocator.getBlocks();
    EXPECT_TRUE(blocks.size() == 1);
    EXPECT_FALSE(blocks[0].isAllocated);

    void* x = allocator.allocate(1024);
    blocks = allocator.getBlocks();

    // Not sure which block index is the one we just allocated, but there should be
    // one of the two.
    EXPECT_TRUE(blocks.size() == 2);
    EXPECT_TRUE(
            (blocks[0].isAllocated && blocks[0].size == 1024) || 
            (blocks[1].isAllocated && blocks[1].size == 1024));
    EXPECT_TRUE(!blocks[0].isAllocated || !blocks[1].isAllocated);

    allocator.release(x);
    blocks = allocator.getBlocks();

    EXPECT_TRUE(blocks.size() == 1);
    EXPECT_FALSE(blocks[0].isAllocated);
}

TEST(HeapAllocator, SmallBinAlloc)
{
    mem::HeapAllocator allocator;
    std::vector<void*> allocs;

    // This tests small allocations from the reserve
    for (int i = 0; i < 256 ; ++i) {
        void* x = allocator.allocate(i);
        allocs.push_back(x);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
    }

    // This fills the small bins with unallocated blocks
    for (void* ptr: allocs) {
        allocator.release(ptr);        
    }
    allocs.clear();

    // Now, allocate again to allocate from the bins rather than the reserve.
    for (int i = 0; i < 256 ; ++i) {
        void* x = allocator.allocate(i);
        allocs.push_back(x);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
    }
    for (void* ptr: allocs) {
        allocator.release(ptr);        
    }
    allocs.clear();
    EXPECT_TRUE(allocator.check());
}

TEST(HeapAllocator, SmallBinCoalescing)
{
    mem::HeapAllocator allocator(util::kilobytes(64), util::bytes(1));;
    std::vector<void*> allocs;

    std::vector<mem::HeapAllocator::Block> allBlocks = allocator.getBlocks();
    EXPECT_EQ(1, allBlocks.size());

    for (int i = 0; i < 256 ; ++i) {
        void* x = allocator.allocate(i);
        allocs.push_back(x);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
    }

    // +1 for reserve
    allBlocks = allocator.getBlocks();
    EXPECT_EQ(257, allBlocks.size());

    for (void* ptr: allocs) {
        allocator.release(ptr);
    }

    // Small bins don't coalesce
    allBlocks = allocator.getBlocks();
    EXPECT_EQ(257, allBlocks.size());
}

TEST(HeapAllocator, SmallBinChains)
{
    // Allocations all the same size, test chaining
    mem::HeapAllocator allocator;
    std::vector<void*> allocs;

    for (int i = 0; i < 256 ; ++i) {
        void* x = allocator.allocate(16);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
        allocs.push_back(x);
    }

    for (int i = 0; i < 256 ; ++i) {
        void* x = allocator.allocate(32);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
        allocs.push_back(x);
    }

    for (void* ptr: allocs) {
        allocator.release(ptr);
    }
    EXPECT_TRUE(allocator.check());
}

TEST(HeapAllocator, SmallBinAllocDelete)
{
    mem::HeapAllocator allocator;
    for (int i = 0; i < 256 ; ++i) {
        void* x = allocator.allocate(i);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
        allocator.release(x);
        EXPECT_TRUE(allocator.check());
    }
}

TEST(HeapAllocator, SmallBinStress)
{
    // This test just does a ton of random memory 'events' aka
    // allocations and de-allocations. Use a consistent seed to
    // make this repeatable and debuggable.
    mem::HeapAllocator allocator(1024);
    std::vector<void*> allocs;

    const size_t RandSeed = 117;
    srand(RandSeed);

    const double AllocChance = 0.70;

    const size_t NumEvents = 10000;
    for (size_t i = 0; i < NumEvents; ++i) {
        // Determine if we are allocating or freeing
        if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
            size_t numBytes = rand()%256;
            void* x = allocator.allocate(numBytes);
            allocs.push_back(x);
            EXPECT_TRUE(allocator.check());
        } else {
            size_t releaseIndex = rand()%allocs.size();
            allocator.release(allocs[releaseIndex]);
            allocs.erase(allocs.begin() + releaseIndex);
            EXPECT_TRUE(allocator.check());
        }
    }
    
    // Free any remaining
    EXPECT_TRUE(allocator.check());
    for (void* ptr: allocs) {
        allocator.release(ptr);
    }
    EXPECT_TRUE(allocator.check());
}

TEST(HeapAllocator, TreeBinAlloc)
{
    // This tries to hit all ranged bins but probably doesn't
    mem::HeapAllocator allocator;
    std::vector<void*> allocs;

    // Tests allocations from reserve
    for (int i = 256; i < util::kilobytes(256); i += util::bytes(256)) {
        void* x = allocator.allocate(i);
        allocs.push_back(x);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
    }

    // See the SmallBinAlloc test for what our strategy is here
    allocator.enableBlockMerging(false);
    for (void* ptr: allocs) {
        allocator.release(ptr);
    }
    allocs.clear();

    for (int i = 256; i < util::kilobytes(256); i += util::bytes(256)) {
        void* x = allocator.allocate(i);
        allocs.push_back(x);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
    }
    for (void* ptr: allocs) {
        allocator.release(ptr);
    }
    EXPECT_TRUE(allocator.check());
    allocs.clear();
}

TEST(HeapAllocator, TreeBinCoalescing)
{
    mem::HeapAllocator allocator;
    std::vector<void*> allocs;

    std::vector<mem::HeapAllocator::Block> allBlocks = allocator.getBlocks();
    EXPECT_EQ(1, allBlocks.size());

    // Tests allocations from reserve
    for (int i = 256; i < util::kilobytes(256); i += util::bytes(256)) {
        void* x = allocator.allocate(i);
        allocs.push_back(x);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
    }

    allBlocks = allocator.getBlocks();
    EXPECT_TRUE(allBlocks.size() > 1);

    for (void* ptr: allocs) {
        allocator.release(ptr);
    }
    allocs.clear();

    // Should be one again since all the unallocated blocks will coalesce
    allBlocks = allocator.getBlocks();
    EXPECT_EQ(1, allBlocks.size());
}

TEST(HeapAllocator, TreeBinTrees)
{
    // Pick on one bin 45, which has sizes from 24576 to 32767
    // Add 256 random entries and then randomly remove them
    const size_t RandSeed = 125;
    srand(RandSeed);

    const size_t MinValue = 24576;
    const size_t MaxValue = 32767;
    const size_t NumValues = MaxValue - MinValue + 1;

    mem::HeapAllocator allocator;
    std::vector<void*> allocs;

    // Add each size 4 times to test chaining within the trees
    for (int i = 0; i < 256 ; ++i) {
        size_t numBytes = MinValue + rand()%NumValues;
        for (int j = 0; j < 4; ++j) {
            void * x = allocator.allocate(numBytes);
            EXPECT_TRUE(x != nullptr);
            EXPECT_TRUE(allocator.check());
            allocs.push_back(x);
        }
    }

    allocator.enableBlockMerging(false);
    while (!allocs.empty()) {
        size_t i = rand()%allocs.size();
        allocator.release(allocs[i]);
        EXPECT_TRUE(allocator.check());
        allocs.erase(allocs.begin() + i);
    }
}

TEST(HeapAllocator, TreeBinAllocDelete)
{
    mem::HeapAllocator allocator;
    for (int i = 256; i < util::kilobytes(256); i += util::bytes(256)) {
        void* x = allocator.allocate(i);
        EXPECT_TRUE(x != nullptr);
        EXPECT_TRUE(allocator.check());
        allocator.release(x);
        EXPECT_TRUE(allocator.check());
    }
}

TEST(HeapAllocator, TreeBinStress)
{
    // This test just does a ton of random memory 'events' aka
    // allocations and de-allocations. Use a consistent seed to
    // make this repeatable and debuggable.
    mem::HeapAllocator allocator;
    std::vector<void*> allocs;

    const size_t RandSeed = 121;
    srand(RandSeed);

    // Favour deallocations so we don't run out of memory
    const double AllocChance = 0.50;

    const size_t NumEvents = 100000;
    for (size_t i = 0; i < NumEvents; ++i) {
        // Determine if we are allocating or freeing
        if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
            size_t numBytes = rand()%util::bytes(16777215);
            void* x = allocator.allocate(numBytes);
            allocs.push_back(x);
            EXPECT_TRUE(allocator.check());
        } else {
            size_t releaseIndex = rand()%allocs.size();
            allocator.release(allocs[releaseIndex]);
            allocs.erase(allocs.begin() + releaseIndex);
            EXPECT_TRUE(allocator.check());
        }
    }
    
    // Free any remaining
    EXPECT_TRUE(allocator.check());
    for (void* ptr: allocs) {
        allocator.release(ptr);
    }
    EXPECT_TRUE(allocator.check());
}

TEST(HeapAllocator, LargeAlloc)
{
    mem::HeapAllocator allocator;

    // Make allocations large enough to trigger an external
    // segment to be created.
    EXPECT_EQ(0, allocator.getStats().numExternalSegments);
    void* big = allocator.allocate(util::megabytes(35));
    EXPECT_EQ(1, allocator.getStats().numExternalSegments);
    allocator.allocate(util::kilobytes(1));
    EXPECT_EQ(1, allocator.getStats().numExternalSegments);
    allocator.allocate(util::megabytes(33));
    EXPECT_EQ(2, allocator.getStats().numExternalSegments);
    allocator.release(big);
    EXPECT_EQ(1, allocator.getStats().numExternalSegments);
}

TEST(HeapAllocator, Alignment)
{
    const size_t NumEvents = 10000;
    std::vector<void*> allocs;

    const float AllocChance = 0.7;

    // Best way to test is just to do a bunch of allocations/deletions
    // and ensure the alignment is right. 4 and 8 byte alignment
    // is most common, just test those.
    {
        mem::HeapAllocator allocator4(util::kilobytes(64), 4);

        for (size_t i = 0; i < NumEvents; ++i) {
            // Determine if we are allocating or freeing
            if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
                size_t numBytes = rand()%util::bytes(16777215);
                void* x = allocator4.allocate(numBytes);
                allocs.push_back(x);
                EXPECT_TRUE(allocator4.check());
                EXPECT_TRUE((size_t)x%4 == 0);
            } else {
                size_t releaseIndex = rand()%allocs.size();
                allocator4.release(allocs[releaseIndex]);
                allocs.erase(allocs.begin() + releaseIndex);
                EXPECT_TRUE(allocator4.check());
            }
        }
        
        // Free any remaining
        for (void* ptr: allocs) {
            allocator4.release(ptr);
        }
        EXPECT_TRUE(allocator4.check());
        allocs.clear();
    }

    {
        mem::HeapAllocator allocator8(util::kilobytes(64), 8);

        for (size_t i = 0; i < NumEvents; ++i) {
            // Determine if we are allocating or freeing
            if (allocs.empty() || static_cast<double>(rand())/RAND_MAX < AllocChance) {
                size_t numBytes = rand()%util::bytes(16777215);
                void* x = allocator8.allocate(numBytes);
                allocs.push_back(x);
                EXPECT_TRUE(allocator8.check());
                EXPECT_TRUE((size_t)x%8 == 0);
            } else {
                size_t releaseIndex = rand()%allocs.size();
                allocator8.release(allocs[releaseIndex]);
                allocs.erase(allocs.begin() + releaseIndex);
                EXPECT_TRUE(allocator8.check());
            }
        }
        
        // Free any remaining
        for (void* ptr: allocs) {
            allocator8.release(ptr);
        }
        EXPECT_TRUE(allocator8.check());
        allocs.clear();
    }

    // Other cases to consider are external segments (with deletion)
    mem::HeapAllocator allocator(util::kilobytes(64), 16);
    void* big1 = allocator.allocate(util::bytes(40971520));
    void* big2 = allocator.allocate(util::bytes(40971521));
    void* big3 = allocator.allocate(util::bytes(40971522));
    EXPECT_TRUE((size_t)big1%16 == 0);
    EXPECT_TRUE((size_t)big2%16 == 0);
    EXPECT_TRUE((size_t)big3%16 == 0);
}

