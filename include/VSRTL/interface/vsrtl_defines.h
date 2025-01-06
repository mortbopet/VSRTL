#pragma once

#include <cstdint>
#include <limits.h>
#include <type_traits>

namespace vsrtl {
using VSRTL_VT_U = uint64_t;
using VSRTL_VT_S = int64_t;
static_assert(CHAR_BIT == 8, "VSRTL only supports systems with CHAR_BIT = 8");
static_assert(sizeof(VSRTL_VT_S) == sizeof(VSRTL_VT_U),
              "Base value types must be equal in size");
static_assert(std::is_unsigned<VSRTL_VT_U>::value,
              "VSRTL_VT_U must be an unsigned data type");
static_assert(std::is_signed<VSRTL_VT_S>::value,
              "VSRTL_VT_S must be a signed data type");

constexpr VSRTL_VT_U VSRTL_VT_BITS = sizeof(VSRTL_VT_U) * CHAR_BIT;

template <typename T>
constexpr VSRTL_VT_U VT_U(const T &v) {
  return static_cast<VSRTL_VT_U>(v);
}
template <typename T>
constexpr VSRTL_VT_S VT_S(const T &v) {
  return static_cast<VSRTL_VT_S>(v);
}
} // namespace vsrtl
