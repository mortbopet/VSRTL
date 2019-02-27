#ifndef VSRTL_NESTEDEXPONENTER_H
#define VSRTL_NESTEDEXPONENTER_H

#include "vsrtl_alu.h"
#include "vsrtl_component.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_register.h"

namespace vsrtl {

class Exponenter : public Component {
public:
    Exponenter(std::string name) : Component(name) {
        expIn.setWidth(32);
        out.setWidth(32);

        mul->out >> expReg->in;

        expIn >> mul->op1;
        expIn >> mul->op2;
        aluOp->out >> mul->ctrl;

        expReg->out >> out;
    }
    INPUTPORT(expIn);
    OUTPUTPORT(out);

    SUBCOMPONENT(expReg, Register, 32);
    SUBCOMPONENT(mul, ALU, 32);
    SUBCOMPONENT(aluOp, Constant, ALU_OPCODE::MUL, ALUctrlWidth());
};

class NestedExponenter : public Design {
public:
    NestedExponenter() : Design("Nested Exponenter") {
        exp->out >> adder->op1;
        reg->out >> adder->op2;
        aluOp->out >> adder->ctrl;

        add2->out >> reg->in;
        adder->out >> exp->expIn;

        adder->out >> add2->op1;
        c2->out >> add2->op2;
        aluOp->out >> add2->ctrl;
    }
    // Create objects
    SUBCOMPONENT(exp, Exponenter);
    SUBCOMPONENT(reg, Register, 32);
    SUBCOMPONENT(adder, ALU, 32);
    SUBCOMPONENT(add2, ALU, 32);
    SUBCOMPONENT(c2, Constant, 2, 32);
    SUBCOMPONENT(aluOp, Constant, ALU_OPCODE::ADD, ALUctrlWidth());
};
}  // namespace vsrtl

#endif  // VSRTL_NESTEDEXPONENTER_H
