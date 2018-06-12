#ifndef RIPESDEFINES_H
#define RIPESDEFINES_H

namespace ripes {

#ifdef RIPES64
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
#endif  // RIPESDEFINES_H
