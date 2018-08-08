#include "catch.hpp"

#include "ripes_alu.h"
#include "ripes_architecture.h"
#include "ripes_component.h"
#include "ripes_constant.h"
#include "ripes_register.h"

namespace ripes {

class Exponenter : public Component {
    NON_REGISTER_COMPONENT
public:
    SUBCOMPONENT(reg, Register, 32);
    SUBCOMPONENT(mul, ALU, 32);
    SUBCOMPONENT(aluOp, Constant, ALUctrlWidth(), ALU_OPCODE::MUL);

    INPUTSIGNAL(in, 32);
    OUTPUTSIGNAL(out, 32);

    Exponenter() : Component("Exponenter") {
        connect(mul->out, reg->in);

        connect(out, in);

        connect(in, mul->op1);
        connect(in, mul->op2);
        connect(aluOp->out, mul->ctrl);

        out->setPropagationFunction(reg->out->getFunctor());
    }
};

class tst_nestedComponents : public Architecture<0> {
public:
    // Create objects
    SUBCOMPONENT_NT(exp, Exponenter);
    SUBCOMPONENT(reg, Register, 32);
    SUBCOMPONENT(adder, ALU, 32);
    SUBCOMPONENT(add2, ALU, 32);
    SUBCOMPONENT(c2, Constant, 32, 2);
    SUBCOMPONENT(aluOp, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);

    tst_nestedComponents() {
        connect(exp->out, adder->op1);
        connect(reg->out, adder->op2);
        connect(aluOp->out, adder->ctrl);

        connect(add2->out, reg->in);
        connect(adder->out, exp->in);

        connect(adder->out, add2->op1);
        connect(c2->out, add2->op2);
        connect(aluOp->out, add2->ctrl);
    }
};
}  // namespace ripes

TEST_CASE("Test nested components") {
    ripes::tst_nestedComponents a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();

    const int n = 10;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    // REQUIRE(a.regs->value(5) == 40);
}
