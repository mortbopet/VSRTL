#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

template <OpType T_op>
class OpCvt : public Component {
public:
    SetGraphicsType(Wire);
    OpCvt(std::string name, SimComponent* parent, unsigned int w) : Component(name, parent) {
        DYNP_IN_INIT(in, w);

        if constexpr (T_op == OpType::Signed) {
            DYNP_OUT_INIT(out, w);

        } else if constexpr (T_op == OpType::Unsigned) {
            DYNP_OUT_INIT(out, w + 1);
        } else {
            throw std::runtime_error("Unimplemented");
        }
        *out << [=] { return in->sValue(); };
    }

    DYNP_IN(in);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
