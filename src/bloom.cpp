// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

#include "bloom.hpp"

#include <openssl/evp.h>

#include <bit>
#include <cstdlib>
#include <cstring>

using namespace util;

Bloom::Bloom() :
    Bloom(kDefaultHashFunctionCount)
{}

Bloom::Bloom(size_t hashFunctionCount) :
    objectCount_(0),
    hashFunctionCount_(hashFunctionCount)
{}

std::expected<Bloom, Bloom::Error> Bloom::create(size_t hashFunctionCount)
{
    if (0 == hashFunctionCount) {
        return std::unexpected(Error::kNoHashFunctions);
    }
    if (kDigestSize < hashFunctionCount * kBytesPerHashFunction) {
        return std::unexpected(Error::kTooManyHashFunctions);
    }
    return Bloom(hashFunctionCount);
}

void Bloom::insert(const std::string& object)
{
    const Digest digest = hash(object);
    for (size_t idx = 0; idx < hashFunctionCount_; idx++) {
        bloomfilterStore_[bitIndex(digest, idx)] = true;
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
    const Digest digest = hash(object);
    for (size_t idx = 0; idx < hashFunctionCount_; idx++) {
        if (!bloomfilterStore_[bitIndex(digest, idx)])
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

Bloom::Digest Bloom::hash(const std::string& object)
{
    Digest digest{};
    unsigned int digestLength = 0;
    // EVP_Digest only fails when OpenSSL cannot allocate its context, and an
    // insert or lookup has nothing useful to return then.
    if (EVP_Digest(object.data(), object.size(), digest.data(), &digestLength, EVP_sha256(),
            nullptr) != 1 ||
        digestLength != kDigestSize) {
        std::abort();
    }
    return digest;
}

std::uint16_t Bloom::bitIndex(const Digest& digest, size_t hashFunction)
{
    // memcpy rather than a uint16_t* into the byte array, which broke strict
    // aliasing. The bytes are read as little-endian so a filter means the same
    // bits on every host.
    std::uint16_t index = 0;
    std::memcpy(&index, digest.data() + hashFunction * kBytesPerHashFunction, sizeof(index));
    if constexpr (std::endian::native == std::endian::big) {
        index = std::byteswap(index);
    }
    return index;
}
