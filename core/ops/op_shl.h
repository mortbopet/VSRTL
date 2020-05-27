#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

class OpShl : public Component {
public:
    SetGraphicsType(Wire);
    OpShl(std::string name, SimComponent* parent, unsigned int w, unsigned int n) : Component(name, parent) {
        DYNP_IN_INIT(in, w);
        const unsigned w_out = w + n;
        DYNP_OUT_INIT(out, w_out);
        *out << [=] { return in->uValue() << n; };
    }

    DYNP_IN(in);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
