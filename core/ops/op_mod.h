#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

template <OpType T_op>
class OpMod : public Component {
public:
    SetGraphicsType(Adder);
    OpMod(std::string name, SimComponent* parent, unsigned int w_num, unsigned int w_den) : Component(name, parent) {
        DYNP_IN_INIT(num, w_num);
        DYNP_IN_INIT(den, w_den);

        const unsigned w_out = std::min(w_num, w_den);
        DYNP_OUT_INIT(out, w_out);

        if constexpr (T_op == OpType::Signed) {
            *out << [=] { return num->template value<VSRTL_VT_S>() % den->template value<VSRTL_VT_S>(); };
        } else if constexpr (T_op == OpType::Unsigned) {
            *out << [=] { return num->template value<VSRTL_VT_U>() % den->template value<VSRTL_VT_U>(); };
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
