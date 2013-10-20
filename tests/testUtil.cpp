#include <gtest/gtest.h>
#include <string>

#include "mem/util.h"

TEST(UtilTest, NextAlignedAddress)
{
    EXPECT_EQ((void*)1000, mem::align((void*)1000, 4));
    EXPECT_EQ((void*)1004, mem::align((void*)1001, 4));
    EXPECT_EQ((void*)1004, mem::align((void*)1002, 4));
    EXPECT_EQ((void*)1004, mem::align((void*)1003, 4));
    EXPECT_EQ((void*)1004, mem::align((void*)1004, 4));
    EXPECT_EQ((void*)1000, mem::align((void*)999, 4));
}

