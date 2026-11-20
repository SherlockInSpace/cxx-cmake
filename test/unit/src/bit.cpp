// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

#include <util/bit.hpp>

#include <gtest/gtest.h>

/************************************ TESTS ***********************************/
TEST(min, zero)
{
    EXPECT_EQ(0, util::bit::min(0, 0));
}

TEST(min, negative)
{
    EXPECT_EQ(-1, util::bit::min(0, -1));
    EXPECT_EQ(-1, util::bit::min(-1, 0));
    EXPECT_EQ(-2, util::bit::min(-1, -2));
    EXPECT_EQ(-2, util::bit::min(-2, -1));
    EXPECT_EQ(-1, util::bit::min(-1, -1));
}

TEST(min, positive)
{
    EXPECT_EQ(0, util::bit::min(0, 1));
    EXPECT_EQ(0, util::bit::min(1, 0));
    EXPECT_EQ(1, util::bit::min(1, 2));
    EXPECT_EQ(1, util::bit::min(2, 1));
    EXPECT_EQ(1, util::bit::min(1, 1));
}

TEST(max, zero)
{
    EXPECT_EQ(0, util::bit::max(0, 0));
}

TEST(max, negative)
{
    EXPECT_EQ(0, util::bit::max(0, -1));
    EXPECT_EQ(0, util::bit::max(-1, 0));
    EXPECT_EQ(-1, util::bit::max(-1, -2));
    EXPECT_EQ(-1, util::bit::max(-2, -1));
    EXPECT_EQ(-1, util::bit::max(-1, -1));
}

TEST(max, positive)
{
    EXPECT_EQ(1, util::bit::max(0, 1));
    EXPECT_EQ(1, util::bit::max(1, 0));
    EXPECT_EQ(2, util::bit::max(1, 2));
    EXPECT_EQ(2, util::bit::max(2, 1));
    EXPECT_EQ(1, util::bit::max(1, 1));
}

// Both are constexpr, so they work where a constant expression is needed.
static_assert(util::bit::min(3, 7) == 3);
static_assert(util::bit::max(3, 7) == 7);

TEST(min, unsignedAndWide)
{
    EXPECT_EQ(2U, util::bit::min(2U, 9U));
    EXPECT_EQ(-5LL, util::bit::min(-5LL, 1LL << 40));
}

TEST(max, unsignedAndWide)
{
    EXPECT_EQ(9U, util::bit::max(2U, 9U));
    EXPECT_EQ(1LL << 40, util::bit::max(-5LL, 1LL << 40));
}
