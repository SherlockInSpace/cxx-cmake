#include "bloom.hpp"

#include <gtest/gtest.h>

#include <vector>

/************************************ TESTS ***********************************/
TEST(bloom, exceptHashFunctionCount)
{
    EXPECT_THROW(util::Bloom(0), std::invalid_argument);
}

TEST(bloom, createInsertCountClear)
{
    util::Bloom* bloomFilter = new util::Bloom();

    // Is the current object count 0?
    EXPECT_EQ(0, bloomFilter->objectCount());

    std::vector<std::string> test_strings = {"Hello", "World", "!", "My", "name", "is", "Bloom!"};

    // Add each string and check the objectCount matches
    for (auto idx = 0; idx < test_strings.size(); ++idx) {
        bloomFilter->insert(test_strings[idx]);
        EXPECT_EQ(idx + 1, bloomFilter->objectCount());
    }

    // clear the hash table
    bloomFilter->clear();
    EXPECT_EQ(0, bloomFilter->objectCount());
}

TEST(bloom, contains)
{
    util::Bloom* bloomFilter = new util::Bloom();

    // Is the current object count 0?
    EXPECT_EQ(0, bloomFilter->objectCount());

    std::vector<std::string> test_strings = {"Hello", "World", "!", "My", "name", "is", "Bloom!"};

    // Add each string
    for (auto idx = 0; idx < test_strings.size(); ++idx) {
        bloomFilter->insert(test_strings[idx]);
    }

    // Check each string exists
    for (auto& str : test_strings) {
        EXPECT_EQ(true, bloomFilter->contains(str));
    }

    // Check for string that does NOT exist
    EXPECT_EQ(false, bloomFilter->contains("space"));
}

TEST(bloom, collision)
{
    util::Bloom* bloomFilter = new util::Bloom();

    std::string msg1 = "\xd1\x31\xdd\x02\xc5\xe6\xee\xc4"
                       "\x69\x3d\x9a\x06\x98\xaf\xf9\x5c"
                       "\x2f\xca\xb5\x87\x12\x46\x7e\xab"
                       "\x40\x04\x58\x3e\xb8\xfb\x7f\x89"
                       "\x55\xad\x34\x06\x09\xf4\xb3\x02"
                       "\x83\xe4\x88\x83\x25\x71\x41\x5a"
                       "\x08\x51\x25\xe8\xf7\xcd\xc9\x9f"
                       "\xd9\x1d\xbd\xf2\x80\x37\x3c\x5b"
                       "\xd8\x82\x3e\x31\x56\x34\x8f\x5b"
                       "\xae\x6d\xac\xd4\x36\xc9\x19\xc6"
                       "\xdd\x53\xe2\xb4\x87\xda\x03\xfd"
                       "\x02\x39\x63\x06\xd2\x48\xcd\xa0"
                       "\xe9\x9f\x33\x42\x0f\x57\x7e\xe8"
                       "\xce\x54\xb6\x70\x80\xa8\x0d\x1e"
                       "\xc6\x98\x21\xbc\xb6\xa8\x83\x93"
                       "\x96\xf9\x65\x2b\x6f\xf7\x2a\x70";

    std::string msg2 = "\xd1\x31\xdd\x02\xc5\xe6\xee\xc4"
                       "\x69\x3d\x9a\x06\x98\xaf\xf9\x5c"
                       "\x2f\xca\xb5\x07\x12\x46\x7e\xab"
                       "\x40\x04\x58\x3e\xb8\xfb\x7f\x89"
                       "\x55\xad\x34\x06\x09\xf4\xb3\x02"
                       "\x83\xe4\x88\x83\x25\xf1\x41\x5a"
                       "\x08\x51\x25\xe8\xf7\xcd\xc9\x9f"
                       "\xd9\x1d\xbd\x72\x80\x37\x3c\x5b"
                       "\xd8\x82\x3e\x31\x56\x34\x8f\x5b"
                       "\xae\x6d\xac\xd4\x36\xc9\x19\xc6"
                       "\xdd\x53\xe2\x34\x87\xda\x03\xfd"
                       "\x02\x39\x63\x06\xd2\x48\xcd\xa0"
                       "\xe9\x9f\x33\x42\x0f\x57\x7e\xe8"
                       "\xce\x54\xb6\x70\x80\x28\x0d\x1e"
                       "\xc6\x98\x21\xbc\xb6\xa8\x83\x93"
                       "\x96\xf9\x65\xab\x6f\xf7\x2a\x70";

    // insert message 1
    bloomFilter->insert(msg1);

    // check that string 1 is present
    EXPECT_EQ(true, bloomFilter->contains(msg1));

    // check that string 2 is present (collision)
    EXPECT_EQ(true, bloomFilter->contains(msg2));

    // check that another string does not produce a collision
    EXPECT_EQ(false, bloomFilter->contains("hello"));
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
