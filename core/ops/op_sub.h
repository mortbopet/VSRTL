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
    OpSub(std::string name, SimComponent* parent, unsigned int W) : Component(name, parent) {
        DYNP_IN_INIT(op1, W);
        DYNP_IN_INIT(op2, W);
        DYNP_OUT_INIT(out, W);

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
