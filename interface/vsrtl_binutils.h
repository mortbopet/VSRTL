#pragma once

#include <assert.h>
#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>
#include "limits.h"

namespace vsrtl {

// Sign extension of arbitrary bitfield size.
// Courtesy of http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
template <typename T, unsigned B>
inline T signextend(const T x) {
    struct {
        T x : B;
    } s;
    return s.x = x;
}

// Runtime signextension
template <typename T>
inline T signextend(const T x, unsigned B) {
    int const m = CHAR_BIT * sizeof(T) - B;
    return (x << m) >> m;
}

constexpr inline unsigned generateBitmask(int n) {
    if (n == 0) {
        return 0;
    }
    return static_cast<unsigned>((1 << n) - 1);
}

constexpr inline uint32_t bitcount(int n) {
    unsigned count = 0;
    while (n > 0) {
        count += 1;
        n = n & (n - 1);
    }
    return count;
}

template <uint32_t width>
inline uint32_t accBVec(const std::array<bool, width>& v) {
    uint32_t r = 0;
    for (auto i = 0; i < width; i++) {
        r |= v[i] << i;
    }
    return r;
}

template <unsigned int width>
inline std::array<bool, width> buildUnsignedArr(uint32_t v) {
    std::array<bool, width> r;
    for (size_t i = 0; i < width; i++) {
        r[i] = v & 0b1;
        v >>= 1;
    }
    return r;
}

constexpr inline unsigned floorlog2(unsigned x) {
    return x == 1 ? 0 : 1 + floorlog2(x >> 1);
}

constexpr inline unsigned ceillog2(unsigned x) {
    return x == 1 || x == 0 ? 1 : floorlog2(x - 1) + 1;
}

constexpr unsigned bitsToRepresentValue(int value) {
    const int v = value < 0 ? -value : value;
    const unsigned v_width = ceillog2(v) + ((bitcount(value) == 1) && (value != 0) && (value != 1) ? 1 : 0);
    return v_width;
}

constexpr bool valueFitsInBitWidth(unsigned int width, int value) {
    return bitsToRepresentValue(value) <= width;
}

}  // namespace vsrtl
