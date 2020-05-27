#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

template <OpType T_op>
class OpPad : public Component {
public:
    SetGraphicsType(Wire);
    OpPad(std::string name, SimComponent* parent, unsigned int w, unsigned int n) : Component(name, parent) {
        DYNP_IN_INIT(in, w);

        if (w >= n) {
            DYNP_OUT_INIT(out, w);
            *out << [=] { return in->uValue(); };
        } else {
            DYNP_OUT_INIT(out, n);
            if constexpr (T_op == OpType::Signed) {
                *out << [=] { return in->sValue(); };
            } else if constexpr (T_op == OpType::Unsigned) {
                *out << [=] { return in->uValue(); };
            } else {
                throw std::runtime_error("Unimplemented");
            }
        }
    }

    DYNP_IN(in);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
