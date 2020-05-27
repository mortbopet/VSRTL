#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

template <OpType T_op>
class OpSub : public Component {
public:
    SetGraphicsType(Adder);
    OpSub(std::string name, SimComponent* parent, unsigned int w_op1, unsigned int w_op2) : Component(name, parent) {
        DYNP_IN_INIT(op1, w_op1);
        DYNP_IN_INIT(op2, w_op2);
        const unsigned int w_out = std::max(w_op1, w_op2) + 1;
        DYNP_OUT_INIT(out, w_out);

        if constexpr (T_op == OpType::Signed) {
            *out << [=] { return op1->template value<VSRTL_VT_S>() - op2->template value<VSRTL_VT_S>(); };
        } else if constexpr (T_op == OpType::Unsigned) {
            *out << [=] { return op1->template value<VSRTL_VT_U>() - op2->template value<VSRTL_VT_U>(); };
        } else {
            throw std::runtime_error("Unimplemented");
        }
    }

    DYNP_IN(op1);
    DYNP_IN(op2);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
