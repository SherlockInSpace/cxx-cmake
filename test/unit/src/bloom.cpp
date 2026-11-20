// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

#include "bloom.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <vector>

/************************************ TESTS ***********************************/
TEST(bloom, createRejectsZeroHashFunctions)
{
    const auto bloomFilter = util::Bloom::create(0);
    ASSERT_FALSE(bloomFilter.has_value());
    EXPECT_EQ(util::Bloom::Error::kNoHashFunctions, bloomFilter.error());
}

TEST(bloom, createRejectsTooManyHashFunctions)
{
    const auto bloomFilter = util::Bloom::create(17);
    ASSERT_FALSE(bloomFilter.has_value());
    EXPECT_EQ(util::Bloom::Error::kTooManyHashFunctions, bloomFilter.error());
}

TEST(bloom, createAcceptsEveryValidCount)
{
    for (std::size_t count = 1; count <= 16; ++count) {
        auto bloomFilter = util::Bloom::create(count);
        ASSERT_TRUE(bloomFilter.has_value()) << "count " << count;
        bloomFilter->insert("Hello");
        EXPECT_TRUE(bloomFilter->contains("Hello")) << "count " << count;
        EXPECT_FALSE(bloomFilter->contains("World")) << "count " << count;
    }
}

TEST(bloom, createInsertCountClear)
{
    util::Bloom bloomFilter;

    // Is the current object count 0?
    EXPECT_EQ(0, bloomFilter.objectCount());

    const std::vector<std::string> testStrings = {
        "Hello", "World", "!", "My", "name", "is", "Bloom!"};

    // Add each string and check the objectCount matches
    for (std::size_t idx = 0; idx < testStrings.size(); ++idx) {
        bloomFilter.insert(testStrings[idx]);
        EXPECT_EQ(idx + 1, bloomFilter.objectCount());
    }

    // clear the hash table
    bloomFilter.clear();
    EXPECT_EQ(0, bloomFilter.objectCount());
}

TEST(bloom, contains)
{
    util::Bloom bloomFilter;

    // Is the current object count 0?
    EXPECT_EQ(0, bloomFilter.objectCount());

    const std::vector<std::string> testStrings = {
        "Hello", "World", "!", "My", "name", "is", "Bloom!"};

    // Add each string
    for (const auto& str : testStrings) {
        bloomFilter.insert(str);
    }

    // Check each string exists
    for (const auto& str : testStrings) {
        EXPECT_EQ(true, bloomFilter.contains(str));
    }

    // Check for string that does NOT exist
    EXPECT_EQ(false, bloomFilter.contains("space"));
}

// A Bloom filter promises no false negatives and a false-positive rate near
// (1 - e^(-kn/m))^k whatever its hash is, so this checks those rather than a
// collision pair for one particular hash function.
TEST(bloom, falsePositiveBound)
{
    util::Bloom bloomFilter; // k = 4 over m = 65536 bits
    constexpr int kInserted = 1000;
    for (int i = 0; i < kInserted; ++i) {
        bloomFilter.insert("member-" + std::to_string(i));
    }
    for (int i = 0; i < kInserted; ++i) {
        EXPECT_TRUE(bloomFilter.contains("member-" + std::to_string(i)));
    }

    // (1 - e^(-4 * 1000 / 65536))^4 is about 1.2e-5, so about one hit in
    // 100000 probes. 50 is a wide margin that a broken index still fails.
    constexpr int kProbes = 100000;
    int falsePositives = 0;
    for (int i = 0; i < kProbes; ++i) {
        if (bloomFilter.contains("absent-" + std::to_string(i))) {
            ++falsePositives;
        }
    }
    EXPECT_LE(falsePositives, 50);
}

TEST(bloom, knownNegative)
{
    util::Bloom bloomFilter;
    EXPECT_FALSE(bloomFilter.contains(""));
    EXPECT_FALSE(bloomFilter.contains("hello"));

    bloomFilter.insert("Hello");
    EXPECT_FALSE(bloomFilter.contains("hello"));
}

TEST(bloom, empty)
{
    util::Bloom bloomFilter;

    EXPECT_EQ(true, bloomFilter.empty());

    bloomFilter.insert("Hello");

    EXPECT_EQ(false, bloomFilter.empty());

    bloomFilter.insert("World");

    EXPECT_EQ(false, bloomFilter.empty());

    bloomFilter.clear();

    EXPECT_EQ(true, bloomFilter.empty());
}

// contains() is const and keeps no scratch state, so concurrent lookups on a
// filter nobody is writing to are safe. Run under TSan to check it.
TEST(bloom, concurrentContains)
{
    util::Bloom bloomFilter;
    const std::vector<std::string> keys = {"alpha", "beta", "gamma", "delta", "epsilon"};
    for (const auto& key : keys) {
        bloomFilter.insert(key);
    }

    std::atomic<int> misses{0};
    {
        std::vector<std::jthread> readers;
        for (int reader = 0; reader < 4; ++reader) {
            readers.emplace_back([&bloomFilter, &keys, &misses] {
                for (int round = 0; round < 100; ++round) {
                    for (const auto& key : keys) {
                        if (!bloomFilter.contains(key)) {
                            ++misses;
                        }
                    }
                }
            });
        }
    }
    EXPECT_EQ(0, misses.load());
}
