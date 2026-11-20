// SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
// SPDX-License-Identifier: MIT

/**
 * @file
 * @brief Branchless integer min and max.
 *
 * This is the template's header-only specimen: a module with no source file
 * in src/, which still needs its own test/unit/src/bit.cpp because the
 * test-file check counts public headers as well as sources.
 *
 * In real code prefer std::min and std::max; compilers already emit
 * branchless instructions for them. The other classic bit tricks are in
 * \<bit\> since C++20 as constexpr functions: std::popcount, std::countl_zero,
 * std::countr_zero, std::has_single_bit, std::bit_ceil, std::bit_floor,
 * std::rotl/std::rotr and std::byteswap (C++23).
 */

#pragma once

#include <limits>

namespace util::bit {

/**
 * @brief The smaller of two integers, without a branch.
 *
 * @tparam T An integer type.
 * @param[in] x One value.
 * @param[in] y The other value.
 * @return x if x < y, else y.
 */
template<typename T>
constexpr T min(T x, T y) noexcept
{
    static_assert(std::numeric_limits<T>::is_integer, "Argument must be of type integer!");
    // All ones when x < y, else zero, in T itself so unsigned types mix no int in.
    const T mask = T{0} - static_cast<T>(x < y);
    return y ^ ((x ^ y) & mask);
}

/**
 * @brief The larger of two integers, without a branch.
 *
 * @tparam T An integer type.
 * @param[in] x One value.
 * @param[in] y The other value.
 * @return y if x < y, else x.
 */
template<typename T>
constexpr T max(T x, T y) noexcept
{
    static_assert(std::numeric_limits<T>::is_integer, "Argument must be of type integer!");
    const T mask = T{0} - static_cast<T>(x < y);
    return x ^ ((x ^ y) & mask);
}

} // namespace util::bit
