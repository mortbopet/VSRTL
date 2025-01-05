#ifndef VSRTL_FULLADDER_H
#define VSRTL_FULLADDER_H

#include "vsrtl_design.h"
#include "vsrtl_logicgate.h"

namespace vsrtl {
namespace core {

class FullAdder : public Component {
public:
  FullAdder(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
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

  INPUTPORT(A, 1);
  INPUTPORT(B, 1);
  INPUTPORT(Cin, 1);

  OUTPUTPORT(S, 1);
  OUTPUTPORT(Cout, 1);

  SUBCOMPONENT(xor1, TYPE(Xor<1, 2>));
  SUBCOMPONENT(xor2, TYPE(Xor<1, 2>));
  SUBCOMPONENT(and1, TYPE(And<1, 2>));
  SUBCOMPONENT(and2, TYPE(And<1, 2>));
  SUBCOMPONENT(or1, TYPE(Or<1, 2>));
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_FULLADDER_H
