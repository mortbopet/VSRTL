#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

class OpBitExtr : public Component {
public:
    SetGraphicsType(Wire);
    OpBitExtr(std::string name, SimComponent* parent, unsigned int w_in, unsigned int lo, unsigned int hi)
        : Component(name, parent) {
        if (hi >= w_in || hi >= w_in) {
            throw std::runtime_error(
                "Both hi and lo must be non-negative and strictly less than the bit width of w_in.");
        }
        if (hi < lo) {
            throw std::runtime_error("hi must be greater than or equal to lo.");
        }

        DYNP_IN_INIT(in, w_in);
        const unsigned w_out = hi - lo + 1;
        DYNP_OUT_INIT(out, w_out);

        *out << [=] { return extractBits(in->template value<VSRTL_VT_U>(), w_out, lo); };
    }

    DYNP_IN(in);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
