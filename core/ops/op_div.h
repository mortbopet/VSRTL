#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

template <OpType T_op>
class OpDiv : public Component {
public:
    SetGraphicsType(Adder);
    OpDiv(std::string name, SimComponent* parent, unsigned int w_num, unsigned int w_den) : Component(name, parent) {
        DYNP_IN_INIT(num, w_num);
        DYNP_IN_INIT(den, w_den);

        if constexpr (T_op == OpType::Signed) {
            DYNP_OUT_INIT(out, w_num + 1);
        } else if constexpr (T_op == OpType::Unsigned) {
            DYNP_OUT_INIT(out, w_num);
        } else {
            throw std::runtime_error("Unimplemented");
        }

        if constexpr (T_op == OpType::Signed) {
            *out << [=] { return num->template value<VSRTL_VT_S>() / den->template value<VSRTL_VT_S>(); };
        } else if constexpr (T_op == OpType::Unsigned) {
            *out << [=] { return num->template value<VSRTL_VT_U>() / den->template value<VSRTL_VT_U>(); };
        } else {
            throw std::runtime_error("Unimplemented");
        }
    }

    DYNP_IN(num);
    DYNP_IN(den);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
