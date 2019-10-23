#ifndef VSRTL_ENUM_H
#define VSRTL_ENUM_H

#include "vsrtl_binutils.h"

#include "better-enums/enum.h"

#include <string>

namespace vsrtl {

#define Enum(name, ...) BETTER_ENUM(name, int, __VA_ARGS__);

#define Switch(signal, enumname) switch (signal.value<unsigned int>())

}  // namespace vsrtl

#endif  // VSRTL_ENUM_H
