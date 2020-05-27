#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

template <OpType T_op>
class OpShr : public Component {
public:
    SetGraphicsType(Wire);
    OpShr(std::string name, SimComponent* parent, unsigned int w, unsigned int n) : Component(name, parent) {
        DYNP_IN_INIT(in, w);
        const unsigned w_out = std::max(static_cast<int>(w) - static_cast<int>(n), 1);
        DYNP_OUT_INIT(out, w_out);

        if constexpr (T_op == OpType::Signed) {
            *out << [=] { return in->sValue() >> n; };
        } else if constexpr (T_op == OpType::Unsigned) {
            *out << [=] { return in->uValue() >> n; };
        }
    }

    DYNP_IN(in);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
