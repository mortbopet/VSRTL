#ifndef VSRTL_NESTEDEXPONENTER_H
#define VSRTL_NESTEDEXPONENTER_H

#include "VSRTL/core/vsrtl_alu.h"
#include "VSRTL/core/vsrtl_component.h"
#include "VSRTL/core/vsrtl_constant.h"
#include "VSRTL/core/vsrtl_design.h"
#include "VSRTL/core/vsrtl_register.h"

namespace vsrtl {
namespace core {

class Exponenter : public Component {
public:
  Exponenter(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    mul->out >> expReg->in;

    expIn >> mul->op1;
    expIn >> mul->op2;
    (static_cast<size_t>(ALU_OPCODE::MUL)) >> mul->ctrl;

    expReg->out >> out;
  }
  INPUTPORT(expIn, 32);
  OUTPUTPORT(out, 32);

  SUBCOMPONENT(expReg, Register<32>);
  SUBCOMPONENT(mul, ALU<32>);
};

class NestedExponenter : public Design {
public:
  NestedExponenter() : Design("Nested Exponenter") {
    exp->out >> adder->op1;
    reg->out >> adder->op2;
    (static_cast<size_t>(ALU_OPCODE::ADD)) >> adder->ctrl;

    add2->out >> reg->in;
    adder->out >> exp->expIn;

    adder->out >> add2->op1;
    2 >> add2->op2;
    (static_cast<size_t>(ALU_OPCODE::ADD)) >> add2->ctrl;
  }
  // Create objects
  SUBCOMPONENT(exp, Exponenter);
  SUBCOMPONENT(reg, Register<32>);
  SUBCOMPONENT(adder, ALU<32>);
  SUBCOMPONENT(add2, ALU<32>);
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_NESTEDEXPONENTER_H
