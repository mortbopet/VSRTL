#include <QtTest/QTest>

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
        connectSignal(mul->out, reg->in);

        connectSignal(out, in);

        connectSignal(in, mul->op1);
        connectSignal(in, mul->op2);
        connectSignal(aluOp->out, mul->ctrl);

        out->setPropagationFunction(reg->out->getFunctor());
    }
};

class NestedComponents : public Architecture<0> {
public:
    // Create objects
    SUBCOMPONENT_NT(exp, Exponenter);
    SUBCOMPONENT(reg, Register, 32);
    SUBCOMPONENT(adder, ALU, 32);
    SUBCOMPONENT(add2, ALU, 32);
    SUBCOMPONENT(c2, Constant, 32, 2);
    SUBCOMPONENT(aluOp, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);

    NestedComponents() {
        connectSignal(exp->out, adder->op1);
        connectSignal(reg->out, adder->op2);
        connectSignal(aluOp->out, adder->ctrl);

        connectSignal(add2->out, reg->in);
        connectSignal(adder->out, exp->in);

        connectSignal(adder->out, add2->op1);
        connectSignal(c2->out, add2->op2);
        connectSignal(aluOp->out, add2->ctrl);
    }
};
}  // namespace ripes

class tst_NestedComponents : public QObject {
    Q_OBJECT private slots : void functionalTest();
};

void tst_NestedComponents::functionalTest() {
    ripes::NestedComponents a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();

    const int n = 10;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    // REQUIRE(a.regs->value(5) == 40);
}
QTEST_MAIN(tst_NestedComponents)
#include "tst_nestedcomponent.moc"
