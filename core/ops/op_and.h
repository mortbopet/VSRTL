#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

template <OpType T_op>
class OpAnd : public Component {
public:
    SetGraphicsType(And);
    OpAnd(std::string name, SimComponent* parent, unsigned int w_op1, unsigned int w_op2) : Component(name, parent) {
        DYNP_IN_INIT(op1, w_op1);
        DYNP_IN_INIT(op2, w_op2);
        DYNP_OUT_INIT(out, std::max(w_op1, w_op2));

        if constexpr (T_op == OpType::Signed) {
            *out << [=] { return op1->sValue() & op2->sValue(); };
        } else if constexpr (T_op == OpType::Unsigned) {
            *out << [=] { return op1->uValue() & op2->uValue(); };
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
