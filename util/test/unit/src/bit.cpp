#include "bit.hpp"

#include <gtest/gtest.h>

/************************************ TESTS ***********************************/
TEST(min, zero)
{
    EXPECT_EQ(0, bit::min(0,0));
}

TEST(min, negative)
{
    EXPECT_EQ(-1, bit::min(0, -1));
    EXPECT_EQ(-1, bit::min(-1, 0));
    EXPECT_EQ(-2, bit::min(-1, -2));
    EXPECT_EQ(-2, bit::min(-2, -1));
    EXPECT_EQ(-1, bit::min(-1, -1));
}

TEST(min, positive)
{
    EXPECT_EQ(0, bit::min(0, 1));
    EXPECT_EQ(0, bit::min(1, 0));
    EXPECT_EQ(1, bit::min(1, 2));
    EXPECT_EQ(1, bit::min(2, 1));
    EXPECT_EQ(1, bit::min(1, 1));
}

TEST(max, zero)
{
    EXPECT_EQ(0, bit::max(0,0));
}

TEST(max, negative)
{
    EXPECT_EQ(0, bit::max(0, -1));
    EXPECT_EQ(0, bit::max(-1, 0));
    EXPECT_EQ(-1, bit::max(-1, -2));
    EXPECT_EQ(-1, bit::max(-2, -1));
    EXPECT_EQ(-1, bit::max(-1, -1));
}

TEST(max, positive)
{
    EXPECT_EQ(1, bit::max(0, 1));
    EXPECT_EQ(1, bit::max(1, 0));
    EXPECT_EQ(2, bit::max(1, 2));
    EXPECT_EQ(2, bit::max(2, 1));
    EXPECT_EQ(1, bit::max(1, 1));
}