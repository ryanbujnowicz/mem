#include <array>

#include <gtest/gtest.h>

#include "mem/marking.h"

TEST(Marking, NoMarking)
{
    std::array<char, 1024> mem;
    std::fill(std::begin(mem), std::end(mem), 5);

    mem::NoMarking marker;
    bool allFive;

    marker.onAllocation(mem.data(), 1024);
    allFive = std::all_of(std::begin(mem), std::end(mem), [](char val){ return val == 5; });
    EXPECT_TRUE(allFive);

    marker.onRelease(mem.data(), 1024);
    allFive = std::all_of(std::begin(mem), std::end(mem), [](char val){ return val == 5; });
    EXPECT_TRUE(allFive);
}

TEST(Marking, Marking)
{
    std::array<char, 1024> mem;
    std::fill(std::begin(mem), std::end(mem), 5);

    mem::Marking marker;

    // We just have to know what the magic bytes are
    marker.onAllocation(mem.data(), 1024);
    EXPECT_EQ(0xC, mem[0]);
    EXPECT_EQ(0xD, mem[1]);
    EXPECT_EQ(0xC, mem[2]);
    EXPECT_EQ(0xD, mem[3]);
    EXPECT_EQ(0xC, mem[4]);

    marker.onRelease(mem.data(), 1024);
    EXPECT_EQ(0xD, mem[0]);
    EXPECT_EQ(0xD, mem[1]);
    EXPECT_EQ(0xD, mem[2]);
    EXPECT_EQ(0xD, mem[3]);
    EXPECT_EQ(0xD, mem[1020]);
    EXPECT_EQ(0xD, mem[1021]);
    EXPECT_EQ(0xD, mem[1022]);
    EXPECT_EQ(0xD, mem[1023]);
}

