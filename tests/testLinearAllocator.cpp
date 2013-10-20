#include <gtest/gtest.h>
#include <string>

#include "mem/linearAllocator.h"

struct SomeStruct
{
    char a,b,c;
};

TEST(LinearAllocator, Alloc)
{
    char mem[1024];
    mem::LinearAllocator allocator(mem, 1024, 1);

    mem::LinearAllocator::Stats info1 = allocator.getStats();
    EXPECT_EQ(0, info1.allocatedBytes);
    EXPECT_EQ(1024, info1.freeBytes);

    SomeStruct* s = (SomeStruct*)allocator.allocate(sizeof(SomeStruct));
    SomeStruct* t = (SomeStruct*)allocator.allocate(sizeof(SomeStruct));
    EXPECT_EQ((char*)(s), mem);
    EXPECT_EQ((char*)(t), mem + sizeof(SomeStruct));

    mem::LinearAllocator::Stats info2 = allocator.getStats();
    EXPECT_EQ(2*sizeof(SomeStruct), info2.allocatedBytes);
    EXPECT_EQ(1024 - 2*sizeof(SomeStruct), info2.freeBytes);
}

TEST(LinearAllocator, AllocAlign)
{
    char mem[1024];
    mem::LinearAllocator allocator(mem, 1024, 8);

    mem::LinearAllocator::Stats info1 = allocator.getStats();
    EXPECT_EQ(0, info1.allocatedBytes);
    EXPECT_EQ(1024, info1.freeBytes);

    SomeStruct* s = (SomeStruct*)allocator.allocate(sizeof(SomeStruct));
    SomeStruct* t = (SomeStruct*)allocator.allocate(sizeof(SomeStruct));
    EXPECT_EQ(mem, (char*)(s));
    EXPECT_EQ((char*)s + 8, (char*)(t));

    mem::LinearAllocator::Stats info2 = allocator.getStats();
    EXPECT_EQ(2*8, info2.allocatedBytes);
    EXPECT_EQ(1024 - 2*8, info2.freeBytes);
}

TEST(LinearAllocator, Release)
{
    char mem[1024];
    mem::LinearAllocator allocator(mem, 1024, 1);

    SomeStruct* s = (SomeStruct*)allocator.allocate(sizeof(SomeStruct));

    // release is a noop in the fast allocator
    allocator.release(s);

    SomeStruct* u = (SomeStruct*)allocator.allocate(sizeof(SomeStruct));
    EXPECT_EQ((char*)u, mem + sizeof(SomeStruct));
}

TEST(LinearAllocator, OutOfMemory)
{
    char* mem[1];
    mem::LinearAllocator allocator(mem, 1);
    SomeStruct* s = (SomeStruct*)allocator.allocate(sizeof(SomeStruct));
    EXPECT_EQ(nullptr, s);
}

TEST(LinearAllocator, Clear)
{
    char* mem[1024];
    mem::LinearAllocator allocator(mem, 1024);
    allocator.allocate(sizeof(SomeStruct));
    allocator.allocate(sizeof(SomeStruct));
    allocator.allocate(sizeof(SomeStruct));

    allocator.clear();

    mem::LinearAllocator::Stats info1 = allocator.getStats();
    EXPECT_EQ(0, info1.allocatedBytes);
    EXPECT_EQ(1024, info1.freeBytes);
}

