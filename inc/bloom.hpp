#pragma once

#include <openssl/md5.h>

#include <bitset>
#include <memory>

namespace util {

class Bloom
{
public:
    /**************************** Constructors ********************************/
    
    /**
     * @brief Construct a new Bloom filter object.
     * 
     * @param[in] hashFunctionCount The size of the hash function.
     */
    Bloom(size_t = 4);

    /******************************* Methods **********************************/

    /**
     * @brief Insert an object into the hash table.
     * 
     * @param[in] object The object to insert into the hash table.
     */
    void insert(const std::string&);

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
    bool contains(const std::string&) const;

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
    ///< The MD5 hash function size in bytes.
    static constexpr size_t kMD5ResultSize = 16;
    ///< The store size of the Bloom filter in bytes.
    static constexpr size_t kBloomFilterStoreSize = 65536;
    ///< The bytes per hash function to use.
    static constexpr size_t kBytesPerHashFunction = 2;

    static_assert(1 << (kBytesPerHashFunction * 8) >= kBloomFilterStoreSize,
		"Not all Bloom filter bits indexable, "
        "increase bytes_per_hash_function or decrease bloomfilter_store_size");

    /******************************* Methods **********************************/

    /**
     * @brief Internal function to calculate the hash value of an object. Stores
     * result in a private member for use.
     * 
     * @param[in] object The object to calculate the hash against.
     */
    void hash(const std::string&) const;

    /******************************* Members **********************************/
    ///< The current number of objects in the hash table.
	size_t objectCount_;
    ///< The size of hash function.
    const size_t hashFunctionCount_;
    ///< The bit-array representing the Bloom filter.
	std::bitset<kBloomFilterStoreSize> bloomfilterStore_;
    ///< Used to store the resulting MD5 hash result of an object.
	const std::unique_ptr<unsigned char[]> md5HashResultBuffer_;
}; // end class Bloom

} // end namespace util