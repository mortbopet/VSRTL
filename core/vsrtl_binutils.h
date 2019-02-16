#pragma once

#include "vsrtl_defines.h"

#include <assert.h>
#include <array>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vector>

namespace vsrtl {

typedef std::function<std::vector<R_UINT>(R_UINT)> bitFieldDecoder;

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

template <typename T, size_t S>
inline constexpr auto sum(const std::array<T, S> arr) {
    std::remove_cv_t<std::remove_reference_t<T>> t = 0;
    for (const auto v : arr) {
        t += v;
    }
    return t;
}

template <size_t size>
bitFieldDecoder generateBitFieldDecoder(std::array<R_UINT, size> bitFields) {
    // Generates functors that can decode a binary number based on the input
    // vector which is supplied upon generation

    // Assert that total bitField size is REGISTERWIDTH sized
    assert(sum(bitFields) == REGISTERWIDTH);

    // Generate vector of <fieldsize,bitmask>
    std::vector<std::pair<uint32_t, uint32_t>> parseVector;

    // Generate bit masks and fill parse vector
    for (const auto& field : bitFields) {
        parseVector.push_back(std::pair<uint32_t, uint32_t>(field, generateBitmask(field)));
    }

    // Create parse functor
    bitFieldDecoder wordParser = [=](uint32_t word) {
        std::vector<uint32_t> parsedWord;
        for (const auto& field : parseVector) {
            parsedWord.insert(parsedWord.begin(), word & field.second);
            word = word >> field.first;
        }
        return parsedWord;
    };

    return wordParser;
}
}  // namespace vsrtl
