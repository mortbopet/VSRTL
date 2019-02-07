#ifndef VSRTL_NESTEDEXPONENTER_H
#define VSRTL_NESTEDEXPONENTER_H

#include "vsrtl_alu.h"
#include "vsrtl_component.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_register.h"

namespace vsrtl {

class Exponenter : public Component {
    NON_REGISTER_COMPONENT
public:
    INPUTPORT(expIn, 32);
    OUTPUTPORT(out, 32);

    Exponenter(const char* name) : Component(name) {
        mul->out >> expReg->in;

        expIn >> mul->op1;
        expIn >> mul->op2;
        aluOp->value >> mul->ctrl;

        expReg->out >> out;
    }

    SUBCOMPONENT(expReg, Register, 32);
    SUBCOMPONENT(mul, ALU, 32);
    SUBCOMPONENT(aluOp, Constant, ALUctrlWidth(), ALU_OPCODE::MUL);
};

class NestedExponenter : public Design {
public:
    NestedExponenter() : Design("Nested Exponenter") {
        exp->out >> adder->op1;
        reg->out >> adder->op2;
        aluOp->value >> adder->ctrl;

        add2->out >> reg->in;
        adder->out >> exp->expIn;

        adder->out >> add2->op1;
        c2->value >> add2->op2;
        aluOp->value >> add2->ctrl;
    }
    // Create objects
    SUBCOMPONENT_NT(exp, Exponenter);
    SUBCOMPONENT(reg, Register, 32);
    SUBCOMPONENT(adder, ALU, 32);
    SUBCOMPONENT(add2, ALU, 32);
    SUBCOMPONENT(c2, Constant, 32, 2);
    SUBCOMPONENT(aluOp, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);
};
}  // namespace vsrtl

#endif  // VSRTL_NESTEDEXPONENTER_H
