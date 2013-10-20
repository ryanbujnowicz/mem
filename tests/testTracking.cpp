#include <array>

#include <gtest/gtest.h>

#include "mem/tracking.h"

TEST(Tracking, NoTracking)
{
    std::array<char, 1024> mem;
    std::fill(std::begin(mem), std::end(mem), 5);

    // Test for noop
    mem::NoTracking tracker;
    mem::SourceInfo sourceInfo("test-file.h", 126);
    bool allFive;

    tracker.onAllocation(mem.data(), 1024, 4, sourceInfo);
    allFive = std::all_of(std::begin(mem), std::end(mem), [](char val){ return val == 5; });
    EXPECT_TRUE(allFive);

    tracker.onRelease(mem.data());
    allFive = std::all_of(std::begin(mem), std::end(mem), [](char val){ return val == 5; });
    EXPECT_TRUE(allFive);
}

TEST(Tracking, CountTracking)
{
    std::array<char, 1024> mem;
    std::fill(std::begin(mem), std::end(mem), 5);

    mem::CountTracking tracker;
    EXPECT_EQ(0, tracker.getNumberOfAllocations());
    tracker.onAllocation(mem.data(), 8, 4, mem::SourceInfo("file.h", 22));
    EXPECT_EQ(1, tracker.getNumberOfAllocations());
    tracker.onAllocation(mem.data() + 8, 16, 4, mem::SourceInfo("file.h", 16));
    tracker.onAllocation(mem.data() + 24, 8, 4, mem::SourceInfo("file.h", 12));
    EXPECT_EQ(3, tracker.getNumberOfAllocations());
    tracker.onRelease(mem.data() + 8);
    EXPECT_EQ(2, tracker.getNumberOfAllocations());
    tracker.onAllocation(mem.data() + 32, 8, 4, mem::SourceInfo("file.h", 12));
    tracker.onAllocation(mem.data() + 8, 8, 4, mem::SourceInfo("file.h", 12));
    EXPECT_EQ(4, tracker.getNumberOfAllocations());
    tracker.onRelease(mem.data() + 8);
    tracker.onRelease(mem.data() + 32);
    tracker.onRelease(mem.data() + 24);
    EXPECT_EQ(1, tracker.getNumberOfAllocations());
    tracker.onRelease(mem.data());
    EXPECT_EQ(0, tracker.getNumberOfAllocations());
}

TEST(Tracking, SourceTracking)
{
    std::array<char, 1024> mem;
    std::fill(std::begin(mem), std::end(mem), 5);

    mem::SourceTracking tracker;
    tracker.onAllocation(mem.data(), 8, 4, mem::SourceInfo("file.h", 22));

    mem::TrackingInfo* entry1;
    mem::TrackingInfo* entry2;
    mem::TrackingInfo* entry3;

    entry1 = tracker.getAllocations();
    EXPECT_TRUE(entry1 != nullptr);
    EXPECT_TRUE(!strcmp("file.h", entry1->filename));
    EXPECT_EQ(22, entry1->lineNumber);
    EXPECT_EQ(mem.data(), entry1->mem);
    EXPECT_EQ(8, entry1->size);
    EXPECT_EQ(4, entry1->alignment);
    EXPECT_EQ(0, entry1->callstackSize);
    EXPECT_TRUE(entry1->callstack == nullptr);
    EXPECT_TRUE(entry1->next == nullptr);
    EXPECT_TRUE(entry1->prev == nullptr);

           

    tracker.onAllocation(mem.data() + 8, 8, 8, mem::SourceInfo("file2.h", 101));
    tracker.onAllocation(mem.data() + 16, 16, 4, mem::SourceInfo("file.h", 16));

    entry1 = tracker.getAllocations();
    ASSERT_TRUE(entry1);
    ASSERT_TRUE(entry1->next);
    entry2 = entry1->next;
    ASSERT_TRUE(entry2);
    ASSERT_TRUE(entry2->next);
    entry3 = entry2->next;
    ASSERT_TRUE(entry3);

    EXPECT_TRUE(!strcmp("file.h", entry1->filename));
    EXPECT_EQ(22, entry1->lineNumber);
    EXPECT_EQ(mem.data(), entry1->mem);
    EXPECT_EQ(8, entry1->size);
    EXPECT_EQ(4, entry1->alignment);
    EXPECT_EQ(0, entry1->callstackSize);
    EXPECT_TRUE(entry1->callstack == nullptr);
    EXPECT_EQ(entry2, entry1->next);
    EXPECT_EQ(nullptr, entry1->prev);

    EXPECT_TRUE(!strcmp("file2.h", entry2->filename));
    EXPECT_EQ(101, entry2->lineNumber);
    EXPECT_EQ(mem.data() + 8, entry2->mem);
    EXPECT_EQ(8, entry2->size);
    EXPECT_EQ(8, entry2->alignment);
    EXPECT_EQ(0, entry2->callstackSize);
    EXPECT_TRUE(entry2->callstack == nullptr);
    EXPECT_EQ(entry3, entry2->next);
    EXPECT_EQ(entry1, entry2->prev);

    EXPECT_TRUE(!strcmp("file.h", entry3->filename));
    EXPECT_EQ(16, entry3->lineNumber);
    EXPECT_EQ(mem.data() + 16, entry3->mem);
    EXPECT_EQ(16, entry3->size);
    EXPECT_EQ(4, entry3->alignment);
    EXPECT_EQ(0, entry3->callstackSize);
    EXPECT_TRUE(entry3->callstack == nullptr);
    EXPECT_EQ(nullptr, entry3->next);
    EXPECT_EQ(entry2, entry3->prev);

    tracker.onRelease(mem.data() + 8);
    entry1 = tracker.getAllocations();
    ASSERT_TRUE(entry1);
    ASSERT_TRUE(entry1->next);
    entry2 = entry1->next;
    ASSERT_TRUE(entry2);
    EXPECT_EQ(nullptr, entry2->next);

    EXPECT_EQ(22, entry1->lineNumber);
    EXPECT_EQ(16, entry2->lineNumber);

    tracker.onRelease(mem.data());
    tracker.onRelease(mem.data() + 16);
    EXPECT_EQ(nullptr, tracker.getAllocations());
}

TEST(Tracking, CallStackTracking)
{
    // Much of this is the same as SourceTracking, except the callstack member
    // is populated
    std::array<char, 1024> mem;
    std::fill(std::begin(mem), std::end(mem), 5);

    mem::CallStackTracking tracker;
    tracker.onAllocation(mem.data(), 8, 4, mem::SourceInfo("file.h", 22));

    mem::TrackingInfo* entry1;
    mem::TrackingInfo* entry2;
    mem::TrackingInfo* entry3;

    entry1 = tracker.getAllocations();
    EXPECT_TRUE(entry1 != nullptr);
    EXPECT_TRUE(!strcmp("file.h", entry1->filename));
    EXPECT_EQ(22, entry1->lineNumber);
    EXPECT_EQ(mem.data(), entry1->mem);
    EXPECT_EQ(8, entry1->size);
    EXPECT_EQ(4, entry1->alignment);
    EXPECT_GT(entry1->callstackSize, 0);
    EXPECT_TRUE(entry1->callstack != nullptr);
    EXPECT_TRUE(entry1->next == nullptr);
    EXPECT_TRUE(entry1->prev == nullptr);

    tracker.onAllocation(mem.data() + 8, 8, 8, mem::SourceInfo("file2.h", 101));
    tracker.onAllocation(mem.data() + 16, 16, 4, mem::SourceInfo("file.h", 16));

    entry1 = tracker.getAllocations();
    ASSERT_TRUE(entry1);
    ASSERT_TRUE(entry1->next);
    entry2 = entry1->next;
    ASSERT_TRUE(entry2);
    ASSERT_TRUE(entry2->next);
    entry3 = entry2->next;
    ASSERT_TRUE(entry3);

    EXPECT_TRUE(!strcmp("file.h", entry1->filename));
    EXPECT_EQ(22, entry1->lineNumber);
    EXPECT_EQ(mem.data(), entry1->mem);
    EXPECT_EQ(8, entry1->size);
    EXPECT_EQ(4, entry1->alignment);
    EXPECT_GT(entry1->callstackSize, 0);
    EXPECT_TRUE(entry1->callstack != nullptr);
    EXPECT_EQ(entry2, entry1->next);
    EXPECT_EQ(nullptr, entry1->prev);

    EXPECT_TRUE(!strcmp("file2.h", entry2->filename));
    EXPECT_EQ(101, entry2->lineNumber);
    EXPECT_EQ(mem.data() + 8, entry2->mem);
    EXPECT_EQ(8, entry2->size);
    EXPECT_EQ(8, entry2->alignment);
    EXPECT_GT(entry2->callstackSize, 0);
    EXPECT_TRUE(entry2->callstack != nullptr);
    EXPECT_EQ(entry3, entry2->next);
    EXPECT_EQ(entry1, entry2->prev);

    EXPECT_TRUE(!strcmp("file.h", entry3->filename));
    EXPECT_EQ(16, entry3->lineNumber);
    EXPECT_EQ(mem.data() + 16, entry3->mem);
    EXPECT_EQ(16, entry3->size);
    EXPECT_EQ(4, entry3->alignment);
    EXPECT_GT(entry3->callstackSize, 0);
    EXPECT_TRUE(entry3->callstack != nullptr);
    EXPECT_EQ(nullptr, entry3->next);
    EXPECT_EQ(entry2, entry3->prev);

    tracker.onRelease(mem.data() + 8);
    entry1 = tracker.getAllocations();
    ASSERT_TRUE(entry1);
    ASSERT_TRUE(entry1->next);
    entry2 = entry1->next;
    ASSERT_TRUE(entry2);
    EXPECT_EQ(nullptr, entry2->next);

    EXPECT_EQ(22, entry1->lineNumber);
    EXPECT_EQ(16, entry2->lineNumber);
    EXPECT_NE(entry1->callstack, entry2->callstack);

    tracker.onRelease(mem.data());
    tracker.onRelease(mem.data() + 16);
    EXPECT_EQ(nullptr, tracker.getAllocations());
}

