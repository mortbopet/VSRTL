#ifndef VSRTLDEFINES_H
#define VSRTLDEFINES_H

#include <limits.h>
#include <type_traits>

#include "../external/SparseAddressSpace/SparseAddressSpace.h"
#include "../interface/vsrtl_interface.h"

namespace vsrtl {
namespace core {

// Base value type of ports. Should be set according to the maximally representable number.
using VSRTL_VT_U = unsigned int;
using VSRTL_VT_S = int;
using AddressSpace = SparseAddressSpace<VSRTL_VT_U>;

static_assert(CHAR_BIT == 8, "");
static_assert(sizeof(VSRTL_VT_S) == sizeof(VSRTL_VT_U), "Base value types must be equal in size");
static_assert(std::is_unsigned<VSRTL_VT_U>::value, "VSRTL_VT_U must be an unsigned data type");
static_assert(std::is_signed<VSRTL_VT_S>::value, "VSRTL_VT_S must be a signed data type");

}  // namespace core
}  // namespace vsrtl
#endif  // VSRTLDEFINES_H
