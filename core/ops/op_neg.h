#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

class OpNeg : public Component {
public:
    SetGraphicsType(Wire);
    OpNeg(std::string name, SimComponent* parent, unsigned int w) : Component(name, parent) {
        DYNP_IN_INIT(in, w);
        DYNP_OUT_INIT(in, w + 1);
        *out << [=] { return -in->sValue(); };
    }

    DYNP_IN(in);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
