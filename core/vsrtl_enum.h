#ifndef VSRTL_ENUM_H
#define VSRTL_ENUM_H

#include "vsrtl_binutils.h"

namespace vsrtl {
/** VSRTLEnum
 * Create an enum with an associated width() function, returning the required
 * number of bits to encode the number of enums
 */
#define VSRTLEnum(name, ...)                                                        \
    struct name {                                                                   \
        enum _##name{__VA_ARGS__, _COUNT};                                          \
        static constexpr unsigned int width() { return ceillog2(_##name::_COUNT); } \
    }

#define VSRTLSwitch(signal, enumname) switch (signal.value<unsigned int, enumname::width()>())

}  // namespace vsrtl

#endif  // VSRTL_ENUM_H
