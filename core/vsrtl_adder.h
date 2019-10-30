#pragma once

#include "vsrtl_component.h"
#include "vsrtl_defines.h"

namespace vsrtl {

DefineGraphicsType(Adder);
template <unsigned int W>
class Adder : public Component {
public:
    SetGraphicsType(Adder);
    Adder(std::string name, Component* parent) : Component(name, parent) {
        out << [=] { return op1.template value<VSRTL_VT_S>() + op2.template value<VSRTL_VT_S>(); };
    }

    INPUTPORT(op1, W);
    INPUTPORT(op2, W);
    OUTPUTPORT(out, W);
};
}  // namespace vsrtl
