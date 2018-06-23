#ifndef RISCV_REGISTERFILE_H
#define RISCV_REGISTERFILE_H

#include "ripes_assignable.h"
#include "ripes_defines.h"
#include "ripes_primitive.h"

namespace ripes {

class RISCV_RegisterFile : public RegisterFile {
public:
    RISCV_RegisterFile() {}
};
}  // namespace ripes

#endif  // RISCV_REGISTERFILE_H
