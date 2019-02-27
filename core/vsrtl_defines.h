#ifndef VSRTLDEFINES_H
#define VSRTLDEFINES_H

namespace vsrtl {

// Base value type of ports. Should be set according to the maximally representable number.
using VSRTL_VT_U = unsigned int;
using VSRTL_VT_S = int;
static_assert(sizeof(VSRTL_VT_S) == sizeof(VSRTL_VT_U), "Base value types must be equal in size");

}  // namespace vsrtl
#endif  // VSRTLDEFINES_H
