#ifndef VSRTL_NESTEDEXPONENTER_H
#define VSRTL_NESTEDEXPONENTER_H

#include "vsrtl_alu.h"
#include "vsrtl_architecture.h"
#include "vsrtl_component.h"
#include "vsrtl_constant.h"
#include "vsrtl_register.h"

namespace vsrtl {

class Exponenter : public Component {
    NON_REGISTER_COMPONENT
public:
    INPUTSIGNAL(in, 32);
    OUTPUTSIGNAL(out, 32);

    Exponenter() : Component("Exponenter") {
        connectSignal(mul->out, reg->in);

        connectSignal(out, in);

        connectSignal(in, mul->op1);
        connectSignal(in, mul->op2);
        connectSignal(aluOp->value, mul->ctrl);

        out->setPropagationFunction(reg->out->getFunctor());
    }

    SUBCOMPONENT(reg, Register, 32);
    SUBCOMPONENT(mul, ALU, 32);
    SUBCOMPONENT(aluOp, Constant, ALUctrlWidth(), ALU_OPCODE::MUL);
};

class NestedExponenter : public Architecture {
public:
    NestedExponenter() {
        connectSignal(exp->out, adder->op1);
        connectSignal(reg->out, adder->op2);
        connectSignal(aluOp->value, adder->ctrl);

        connectSignal(add2->out, reg->in);
        connectSignal(adder->out, exp->in);

        connectSignal(adder->out, add2->op1);
        connectSignal(c2->value, add2->op2);
        connectSignal(aluOp->value, add2->ctrl);
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
