#pragma once

#include "vsrtl_defines.h"

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

constexpr unsigned int generateBitmask(int n) {
    return (1 << n) - 1;
}

constexpr uint32_t bitcount(int n) {
    int count = 0;
    while (n > 0) {
        count += 1;
        n = n & (n - 1);
    }
    return count;
}

template <uint32_t width>
uint32_t accBVec(const std::array<bool, width>& v) {
    uint32_t r = 0;
    for (auto i = 0; i < width; i++) {
        r |= v[i] << i;
    }
    return r;
}

template <unsigned int width>
std::array<bool, width> buildUnsignedArr(uint32_t v) {
    std::array<bool, width> r;
    for (size_t i = 0; i < width; i++) {
        r[i] = v & 0b1;
        v >>= 1;
    }
    return r;
}

constexpr unsigned floorlog2(unsigned x) {
    return x == 1 ? 0 : 1 + floorlog2(x >> 1);
}

constexpr unsigned ceillog2(unsigned x) {
    return x == 1 || x == 0 ? 1 : floorlog2(x - 1) + 1;
}

}  // namespace vsrtl
