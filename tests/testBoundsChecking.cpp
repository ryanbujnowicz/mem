#include <array>

#include <gtest/gtest.h>

#include "mem/boundsChecking.h"

TEST(BoundsChecking, NoBoundsChecking)
{
    std::array<char, 1024> mem;
    std::fill(std::begin(mem), std::end(mem), 13);

    // This is a noop, memory should look the same before/after
    mem::NoBoundsChecking boundsChecker;
    bool allThirteen;

    boundsChecker.guardFront(mem.data());
    boundsChecker.guardBack(mem.data() + 1024 - mem::NoBoundsChecking::SizeBack);
    allThirteen = std::all_of(std::begin(mem), std::end(mem), [](char val){ return val == 13; });
    EXPECT_TRUE(allThirteen);

    EXPECT_TRUE(boundsChecker.checkFront(mem.data()));
    EXPECT_TRUE(boundsChecker.checkBack(mem.data() + 1024 - mem::NoBoundsChecking::SizeBack));

    // Mangle memory at front and back
    mem[0] = 123;
    mem[1024 - mem::NoBoundsChecking::SizeBack] = 25;

    EXPECT_TRUE(boundsChecker.checkFront(mem.data()));
    EXPECT_TRUE(boundsChecker.checkBack(mem.data() + 1024 - mem::NoBoundsChecking::SizeBack));
}

TEST(BoundsChecking, BoundsChecking)
{
    std::array<char, 1024> mem;
    std::fill(std::begin(mem), std::end(mem), 13);

    mem::BoundsChecking boundsChecker;
    bool allThirteen;

    boundsChecker.guardFront(mem.data());
    boundsChecker.guardBack(mem.data() + 1024 - mem::BoundsChecking::SizeBack);
    allThirteen = std::all_of(std::begin(mem), std::end(mem), [](char val){ return val == 13; });
    EXPECT_FALSE(allThirteen);

    EXPECT_TRUE(boundsChecker.checkFront(mem.data()));
    EXPECT_TRUE(boundsChecker.checkBack(mem.data() + 1024 - mem::BoundsChecking::SizeBack));

    // Mangle memory at front and back
    mem[0] = 123;
    EXPECT_FALSE(boundsChecker.checkFront(mem.data()));
    EXPECT_TRUE(boundsChecker.checkBack(mem.data() + 1024 - mem::BoundsChecking::SizeBack));
    mem[1024 - mem::BoundsChecking::SizeBack] = 25;
    EXPECT_FALSE(boundsChecker.checkFront(mem.data()));
    EXPECT_FALSE(boundsChecker.checkBack(mem.data() + 1024 - mem::BoundsChecking::SizeBack));
}

