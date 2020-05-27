#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "op_types.h"

namespace vsrtl {
namespace core {

class OpNot : public Component {
public:
    SetGraphicsType(Not);
    OpNot(std::string name, SimComponent* parent, unsigned int w) : Component(name, parent) {
        DYNP_IN_INIT(in, w);
        DYNP_OUT_INIT(in, w);
        *out << [=] { return ~in->uValue(); };
    }

    DYNP_IN(in);
    DYNP_OUT(out);
};
}  // namespace core
}  // namespace vsrtl
