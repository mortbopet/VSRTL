#ifndef VSRTL_ENUM_H
#define VSRTL_ENUM_H

#include "vsrtl_binutils.h"

namespace vsrtl {

#define FIRST_ARG_(N, ...) N

/**
 * @brief The Enum struct
 * Create an enum with an associated width() function, returning the required
 * number of bits to encode the number of enums.
 * @todo: a static assert stating that enum start at 0, and are spaced by 1 (ie. non-user assigned)
 * @todo: an iterater over the enum values
 */
#define Enum(name, ...)                                                                               \
    class name {                                                                                      \
    public:                                                                                           \
        enum _##name{__VA_ARGS__, _COUNT};                                                            \
        static constexpr unsigned int width() { return ceillog2(static_cast<int>(_##name::_COUNT)); } \
        static bool inRange(int v) { return v >= FIRST_ARG_(__VA_ARGS__) && v < _COUNT; }             \
        static constexpr _##name begin() { return FIRST_ARG_(__VA_ARGS__); }                          \
        static constexpr _##name count() { return static_cast<_##name>(_COUNT); }                     \
        static const name& get() {                                                                    \
            static name e;                                                                            \
            return e;                                                                                 \
        }                                                                                             \
                                                                                                      \
    private:                                                                                          \
        name(){};                                                                                     \
    };

#define Switch(signal, enumname) switch (signal.value<unsigned int>())

}  // namespace vsrtl

#endif  // VSRTL_ENUM_H
