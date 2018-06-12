#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace ripes {

#define ASSERT_CONNECTION_DEFINED(primitive)         \
    if (primitive == nullptr) {                      \
        throw std::runtime_error("Undefined input"); \
    }
#define ASSERT_CONNECTION_EXPR(expr)                  \
    if (!(expr)) {                                    \
        throw std::runtime_error("Unverified input"); \
    }

// Sign extension of arbitrary bitfield size.
// Courtesy of http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
template <typename T, unsigned B>
inline T signextend(const T x) {
    struct {
        T x : B;
    } s;
    return s.x = x;
}

constexpr uint32_t generateBitmask(int n) {
    // Generate bitmask. There might be a smarter way to do this
    uint32_t mask = 0;
    for (int i = 0; i < n - 1; i++) {
        mask |= 0b1;
        mask <<= 1;
    }
    mask |= 0b1;
    return mask;
}

constexpr uint32_t bitcount(int n) {
    int count = 0;
    while (n > 0) {
        count += 1;
        n = n & (n - 1);
    }
    return count;
}

template <unsigned int width>
inline uint32_t accBArr(const std::array<bool, width> v) {
    uint32_t r = 0;
    for (size_t i = 0; i < v.size(); i++) {
        r |= v[i] << i;
    }
    return r;
}

template <unsigned int width>
void buildArr(std::array<bool, width>& a, uint32_t v) {
    for (size_t i = 0; i < width; i++) {
        a[i] = v & 0b1;
        v >>= 1;
    }
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

constexpr unsigned floorlog2(unsigned x) {
    return x == 1 ? 0 : 1 + floorlog2(x >> 1);
}

constexpr unsigned ceillog2(unsigned x) {
    return x == 1 || x == 0 ? 1 : floorlog2(x - 1) + 1;
}
}
