#ifndef VSRTLDEFINES_H
#define VSRTLDEFINES_H

#include <limits.h>
#include <type_traits>

namespace vsrtl {

// @todo: describe why this is needed (graphical objects need not know about the templated versions of objects)
#define DefineGraphicsType(name)                                                                         \
    class name##GraphicsProxy {                                                                          \
    public:                                                                                              \
        name##GraphicsProxy() {}                                                                         \
        static std::type_index getTypeIdProxy() { return std::type_index(typeid(name##GraphicsProxy)); } \
    };

#define GraphicsTypeFor(name) (name##GraphicsProxy::getTypeIdProxy())

#define SetGraphicsType(name) \
    std::type_index getTypeId() const override { return GraphicsTypeFor(name); }

// Base value type of ports. Should be set according to the maximally representable number.
using VSRTL_VT_U = unsigned int;
using VSRTL_VT_S = int;
static_assert(CHAR_BIT == 8, "");
static_assert(sizeof(VSRTL_VT_S) == sizeof(VSRTL_VT_U), "Base value types must be equal in size");
static_assert(std::is_unsigned<VSRTL_VT_U>::value, "VSRTL_VT_U must be an unsigned data type");
static_assert(std::is_signed<VSRTL_VT_S>::value, "VSRTL_VT_S must be a signed data type");

}  // namespace vsrtl
#endif  // VSRTLDEFINES_H
