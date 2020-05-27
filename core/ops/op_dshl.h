#pragma once

#include "math.h"

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

class OpDshl : public Component {
public:
    SetGraphicsType(Wire);
    OpDshl(std::string name, SimComponent* parent, unsigned int w_op1, unsigned int w_op2) : Component(name, parent) {
        DYNP_IN_INIT(op1, w_op1);
        DYNP_IN_INIT(op2, w_op2);
        const unsigned w_out = w_op1 + static_cast<unsigned>(std::pow(2, w_op2)) - 1;
        DYNP_OUT_INIT(out, w_out);

        *out << [=] { return op1->uValue() << op2->uValue(); };
    }

    DYNP_IN(op1);
    DYNP_IN(op2);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
