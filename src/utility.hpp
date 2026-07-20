#pragma once

#include <utility>

inline constexpr std::size_t round_nearest_exponent(std::size_t n)
{
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if INTPTR_MAX == UINT64_MAX
    n |= n >> 32;
#endif
    ++n;
    return n;
}
