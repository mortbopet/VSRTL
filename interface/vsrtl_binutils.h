#pragma once

#include <assert.h>
#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>
#include "limits.h"

#include "vsrtl_defines.h"

namespace vsrtl {

// Sign extension of arbitrary bitfield size.
// Courtesy of http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
template <unsigned B, typename T>
inline VSRTL_VT_S signextend(const T x) {
    struct {
        VSRTL_VT_S x : B;
    } s;
    return s.x = x;
}

// Runtime signextension
template <typename T>
inline VSRTL_VT_S signextend(const T x, unsigned B) {
    const int m = CHAR_BIT * sizeof(T) - B;
    return static_cast<VSRTL_VT_S>(x << m) >> m;
}

constexpr VSRTL_VT_U generateBitmask(const VSRTL_VT_U& n) {
    if (n >= (sizeof(VSRTL_VT_U) * CHAR_BIT)) {
        return VT_U(VT_S(-1));
    }
    if (n == 0) {
        return 0;
    }
    return VT_U((1UL << n) - 1);
}

constexpr unsigned bitcount(VSRTL_VT_U n) {
    unsigned count = 0;
    while (n > 0) {
        count += 1;
        n = n & (n - 1);
    }
    return count;
}

template <unsigned width, typename T>
inline T accBVec(const std::array<bool, width>& v) {
    T r = 0;
    for (auto i = 0; i < width; i++) {
        r |= v[i] << i;
    }
    return r;
}

template <unsigned width>
inline std::array<bool, width> buildUnsignedArr(VSRTL_VT_U v) {
    std::array<bool, width> r;
    for (size_t i = 0; i < width; i++) {
        r[i] = v & 0b1;
        v >>= 1;
    }
    return r;
}

template <typename T>
constexpr T floorlog2(const T& x) {
    return x == 1 ? 0 : 1 + floorlog2(x >> 1);
}

template <typename T>
constexpr const T ceillog2(const T& x) {
    return x == 1 || x == 0 ? 1 : floorlog2(x - 1) + 1;
}

}  // namespace vsrtl
