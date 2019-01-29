#include "vsrtl_alu.h"
#include "vsrtl_architecture.h"
#include "vsrtl_constant.h"
#include "vsrtl_register.h"

#include "RISC-V/riscv_registerfile.h"

#include <QtTest/QTest>

static constexpr int resReg = 5;
namespace vsrtl {
/**
 * @brief tst_adderAndReg
 * Small test connecting an ALU, a constant and a register to test clocking of simple circuits
 */
class addOp : public Architecture {
public:
    addOp() {
        // Connect objects

        connectSignal(c4->value, alu->op1);
        connectSignal(alu_ctrl->value, alu->ctrl);
        connectSignal(*regs->operands[0], alu->op2);
        connectSignal(c1->value, regs->writeEnable);
        connectSignal(c5->value, regs->writeRegister);
        connectSignal(alu->out, regs->writeData);
        connectSignal(c_instr->value, regs->instruction);
    }

    // Create objects
    SUBCOMPONENT(alu_ctrl, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);
    SUBCOMPONENT(c5, Constant, 32, resReg);
    SUBCOMPONENT(c4, Constant, 32, 4);
    SUBCOMPONENT(c1, Constant, 32, 1);
    SUBCOMPONENT(c_instr, Constant, 32, resReg << (7 + 5));
    SUBCOMPONENT(alu, ALU, 32);
    SUBCOMPONENT_NT(regs, RISCV_RegisterFile);
};
}  // namespace vsrtl

class tst_AddOp : public QObject {
    Q_OBJECT
private slots:
    void functionalTest();
};

void tst_AddOp::functionalTest() {
    vsrtl::addOp a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();

    const int n = 10;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    // QVERIFY(a.regs->value(5) == 40);
}

QTEST_MAIN(tst_AddOp)
#include "tst_addop.moc"
