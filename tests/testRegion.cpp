#include <cstdlib>

#include <gtest/gtest.h>

#include "mem/mallocAllocator.h"
#include "mem/boundsChecking.h"
#include "mem/region.h"
#include "mem/marking.h"
#include "mem/tracking.h"
#include "mem/threading.h"

typedef mem::Region<
    mem::MallocAllocator,
    mem::SingleThreaded,
    mem::NoBoundsChecking,
    mem::NoTracking,
    mem::NoMarking> 
        SimpleMallocRegion;

SimpleMallocRegion region1;
SimpleMallocRegion region2;

class RegionF : public ::testing::Test
{
public:
    static void SetUpTestCase()
    {
        mem::registerRegion(region1, 1);
        mem::registerRegion(region2, 2);
        mem::setDefaultRegion(2);
    }
};

TEST_F(RegionF, SimpleAllocate)
{
    // This really just tests that this compiles and doesn't crash
    SimpleMallocRegion region;
    void* x = region.allocate(12, 4, mem::SourceInfo("test_file.cpp", 123));
    region.release(x);
}

TEST_F(RegionF, SimpleAlignment)
{
    // This really just tests that this compiles and doesn't crash
    SimpleMallocRegion region;
    void* x;

    x = region.allocate(12, 4, mem::SourceInfo("test_file.cpp", 123));
    EXPECT_EQ(0, (size_t)(x)%4);

    x = region.allocate(113, 8, mem::SourceInfo("test_file.cpp", 123));
    EXPECT_EQ(0, (size_t)(x)%8);

    x = region.allocate(271, 16, mem::SourceInfo("test_file.cpp", 123));
    EXPECT_EQ(0, (size_t)(x)%16);
}

TEST_F(RegionF, GetRegion)
{
    EXPECT_EQ(&region1, &mem::getRegion(1));
    EXPECT_EQ(&region2, &mem::getRegion(2));
}

TEST_F(RegionF, Default)
{
    EXPECT_EQ(&region2, &mem::getRegion(mem::DefaultRegion));
}


