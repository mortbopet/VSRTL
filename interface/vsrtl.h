#pragma once

#include <limits.h>
#include <type_traits>

namespace vsrtl {
using VSRTL_VT_U = unsigned int;
using VSRTL_VT_S = int;
static_assert(CHAR_BIT == 8, "");
static_assert(sizeof(VSRTL_VT_S) == sizeof(VSRTL_VT_U), "Base value types must be equal in size");
static_assert(std::is_unsigned<VSRTL_VT_U>::value, "VSRTL_VT_U must be an unsigned data type");
static_assert(std::is_signed<VSRTL_VT_S>::value, "VSRTL_VT_S must be a signed data type");
}  // namespace vsrtl
