#ifndef VSRTL_FULLADDER_H
#define VSRTL_FULLADDER_H

#include "vsrtl_design.h"
#include "vsrtl_logicgate.h"

namespace vsrtl {

class FullAdder : public Component {
public:
    FullAdder(std::string name, Component* parent) : Component(name, parent) {
        A >> *xor1->in[0];
        B >> *xor1->in[1];

        xor1->out >> *xor2->in[0];
        Cin >> *xor2->in[1];

        xor1->out >> *and1->in[0];
        Cin >> *and1->in[1];

        A >> *and2->in[0];
        B >> *and2->in[1];

        and1->out >> *or1->in[0];
        and2->out >> *or1->in[1];

        or1->out >> Cout;
        xor2->out >> S;
    }

    INPUTPORT_W(A, 1);
    INPUTPORT_W(B, 1);
    INPUTPORT_W(Cin, 1);

    OUTPUTPORT_W(S, 1);
    OUTPUTPORT_W(Cout, 1);

    SUBCOMPONENT(xor1, Xor, 2, 1);
    SUBCOMPONENT(xor2, Xor, 2, 1);
    SUBCOMPONENT(and1, And, 2, 1);
    SUBCOMPONENT(and2, And, 2, 1);
    SUBCOMPONENT(or1, Or, 2, 1);
};
}  // namespace vsrtl

#endif  // VSRTL_FULLADDER_H
