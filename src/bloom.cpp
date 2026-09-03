#include "bloom.hpp"

using namespace util;

Bloom::Bloom(size_t hashFunctionCount) :
    hashFunctionCount_(hashFunctionCount),
    objectCount_(0),
    md5HashResultBuffer_(std::make_unique<unsigned char[]>(kMD5ResultSize))
{
    if (0 == hashFunctionCount) {
        throw std::invalid_argument("Bloomfilter could not be initialized: "
                                    "hashFunctionCount must be larger than 0!");
    }

    if (kMD5ResultSize < hashFunctionCount * kBytesPerHashFunction) {
        throw std::invalid_argument("Bloomfilter could not be initialized: "
                                    "hashFunctionCount too large! "
                                    "hashFunctionCount *  kBytesPerHashFunction must be smaller or "
                                    "equal to kMD5ResultSize");
    }
}

void Bloom::insert(const std::string& object)
{
    hash(object);
    const uint16_t* const objectHashes =
        reinterpret_cast<const uint16_t* const>(md5HashResultBuffer_.get());

    for (size_t idx = 0; idx < hashFunctionCount_; idx++) {
        const uint16_t hashIndex = objectHashes[idx];
        bloomfilterStore_[hashIndex] = true;
    }
    ++objectCount_;
}

void Bloom::clear(void)
{
    bloomfilterStore_.reset();
    objectCount_ = 0;
}

bool Bloom::contains(const std::string& object) const
{
    hash(object);
    const uint16_t* const objectHashes =
        reinterpret_cast<const uint16_t* const>(md5HashResultBuffer_.get());

    for (size_t idx = 0; idx < hashFunctionCount_; idx++) {
        const uint16_t hashIndex = objectHashes[idx];
        if (!bloomfilterStore_[hashIndex])
            return false;
    }
    return true;
}

size_t Bloom::objectCount(void) const
{
    return objectCount_;
}

bool Bloom::empty() const
{
    return 0 == objectCount();
}

void Bloom::hash(const std::string& object) const
{
    const unsigned char* const md5InputVal = reinterpret_cast<const unsigned char*>(object.data());
    const size_t md5InputLength = object.length();
    MD5(md5InputVal, md5InputLength, md5HashResultBuffer_.get());
}
