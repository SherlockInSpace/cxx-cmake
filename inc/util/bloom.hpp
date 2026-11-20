// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>

namespace util {

/**
 * @brief A Bloom filter over strings: a fixed 65536-bit set that answers
 * "possibly present" or "definitely absent".
 *
 * Each object sets one bit per hash function, taken from its SHA-256 digest.
 * contains() never gives a false negative; a false positive becomes likelier
 * as more objects go in. insert() and clear() need exclusive access, while
 * concurrent contains() calls are safe.
 */
class Bloom
{
public:
    /// Why create() refused a hash function count.
    enum class Error
    {
        kNoHashFunctions,     ///< The count was 0.
        kTooManyHashFunctions ///< The count needs more digest bytes than SHA-256 has.
    };

    /**************************** Constructors ********************************/

    /**
     * @brief Construct a Bloom filter with the default 4 hash functions. This
     * cannot fail.
     */
    Bloom();

    /**
     * @brief Construct a Bloom filter with a chosen number of hash functions.
     *
     * @param[in] hashFunctionCount The number of hash functions, 1 to 16.
     * @return The filter, or the reason the count was refused.
     */
    static std::expected<Bloom, Error> create(size_t hashFunctionCount);

    /******************************* Methods **********************************/

    /**
     * @brief Insert an object into the hash table.
     *
     * @param[in] object The object to insert into the hash table.
     */
    void insert(const std::string& object);

    /**
     * @brief Clear out the hash table.
     *
     */
    void clear(void);

    /**
     * @brief Checks whether the object exists in the hash table.
     *
     * @param[in] object The object to check.
     * @return Returns true if the object exists, false otherwise.
     */
    bool contains(const std::string& object) const;

    /**
     * @brief Returns the number of objects in the hash table.
     *
     * @return Returns the number of objects in the hash table.
     */
    size_t objectCount(void) const;

    /**
     * @brief Checks if the hash table is empty or not.
     *
     * @return Returns true if the hash table is empty, false otherwise.
     */
    bool empty(void) const;

private:
    /******************************* Statics **********************************/
    ///< The number of hash functions the default constructor uses.
    static constexpr size_t kDefaultHashFunctionCount = 4;
    ///< The SHA-256 digest size in bytes. Each hash function takes
    ///< kBytesPerHashFunction of it, so a filter has at most 16 hash functions.
    static constexpr size_t kDigestSize = 32;
    ///< The store size of the Bloom filter in bits.
    static constexpr size_t kBloomFilterStoreSize = 65536;
    ///< The bytes per hash function to use.
    static constexpr size_t kBytesPerHashFunction = 2;

    static_assert(kDefaultHashFunctionCount * kBytesPerHashFunction <= kDigestSize,
        "The default constructor must not be able to fail");
    static_assert(1 << (kBytesPerHashFunction * 8) >= kBloomFilterStoreSize,
        "Not all Bloom filter bits indexable, "
        "increase bytes_per_hash_function or decrease bloomfilter_store_size");

    /******************************* Methods **********************************/

    /**
     * @brief Construct a filter whose count create() has already checked.
     *
     * @param[in] hashFunctionCount The number of hash functions, 1 to 16.
     */
    explicit Bloom(size_t hashFunctionCount);

    ///< The digest of one object.
    using Digest = std::array<unsigned char, kDigestSize>;

    /**
     * @brief Hash an object with SHA-256. Touches no member, so concurrent
     * calls are safe. Aborts if OpenSSL cannot compute the digest, which only
     * happens when it cannot allocate.
     *
     * @param[in] object The object to hash.
     * @return The object's digest.
     */
    static Digest hash(const std::string& object);

    /**
     * @brief The filter bit selected by one hash function.
     *
     * @param[in] digest The object's digest.
     * @param[in] hashFunction Which hash function, below hashFunctionCount_.
     * @return The bit index, read little-endian from the digest on every host.
     */
    static std::uint16_t bitIndex(const Digest& digest, size_t hashFunction);

    /******************************* Members **********************************/
    ///< The current number of objects in the hash table.
    size_t objectCount_;
    ///< The size of hash function.
    const size_t hashFunctionCount_;
    ///< The bit-array representing the Bloom filter.
    std::bitset<kBloomFilterStoreSize> bloomfilterStore_;
}; // end class Bloom

} // end namespace util
