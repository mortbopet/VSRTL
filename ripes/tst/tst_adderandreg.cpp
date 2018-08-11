#include "../core/ripes_alu.h"
#include "ripes_architecture.h"
#include "ripes_constant.h"
#include "ripes_register.h"

#include <QtTest/QTest>

namespace ripes {
/**
 * @brief tst_adderAndReg
 * Small test connecting an ALU, a constant and a register to test clocking of simple circuits
 */
class AdderAndReg : public Architecture<3> {
public:
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(alu_ctrl, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);
    SUBCOMPONENT(c4, Constant, 32, 4);
    SUBCOMPONENT(alu, ALU, 32);
    SUBCOMPONENT(reg, Register, 32);

    AdderAndReg() : Architecture() {
        // Connect objects
        connectSignal(c4->out, alu->op1);
        connectSignal(reg->out, alu->op2);
        connectSignal(alu_ctrl->out, alu->ctrl);
        connectSignal(alu->out, reg->in);
    }
};
}  // namespace ripes

class tst_adderAndReg : public QObject {
    Q_OBJECT private slots : void functionalTest();
};

void tst_adderAndReg::functionalTest() {
    ripes::AdderAndReg a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();
    std::cout << "value " << std::endl;
    const int n = 10;
    const int expectedValue = n * a.m_cVal;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    std::cout << "value " << a.reg->out->value<uint32_t>() << std::endl;
    QVERIFY(a.reg->out->value<uint32_t>() == expectedValue);
}

QTEST_MAIN(tst_adderAndReg)
#include "tst_adderandreg.moc"
