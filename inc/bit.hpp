#pragma once

#include <limits>

namespace bit {

template<typename T>
T min(T x, T y)
{
    static_assert(std::numeric_limits<T>::is_integer,
        "Argument must be of type integer!");
    return y ^ ((x ^ y) & -(x < y));
}

template<typename T>
T max(T x, T y)
{
    static_assert(std::numeric_limits<T>::is_integer,
        "Argument must be of type integer!");
    return x ^ ((x ^ y) & -(x < y)); // max(x, y)
}

} // end namespace bit