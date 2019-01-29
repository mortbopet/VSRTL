#ifndef VSRTLDEFINES_H
#define VSRTLDEFINES_H

namespace vsrtl {

#ifdef VSRTL64
#define REGISTERWIDTH 64
#define R_INT int64_t
#define R_UINT uint64_t
#else
#define REGISTERWIDTH 32
#define R_INT int32_t
#define R_UINT uint32_t
#endif
#define REGISTERCOUNT 32
}
#endif  // VSRTLDEFINES_H
