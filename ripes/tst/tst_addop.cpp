#include "ripes_alu.h"
#include "ripes_architecture.h"
#include "ripes_constant.h"
#include "ripes_register.h"

#include "RISC-V/riscv_registerfile.h"

#include "catch.hpp"

static constexpr int resReg = 5;
namespace ripes {
/**
 * @brief tst_adderAndReg
 * Small test connecting an ALU, a constant and a register to test clocking of simple circuits
 */
class tst_addOp : public Architecture<0> {
public:
    // Create objects
    SUBCOMPONENT(alu_ctrl, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);
    SUBCOMPONENT(c5, Constant, 32, resReg);
    SUBCOMPONENT(c4, Constant, 32, 4);
    SUBCOMPONENT(c1, Constant, 32, 1);
    SUBCOMPONENT(c_instr, Constant, 32, resReg << (7 + 5));
    SUBCOMPONENT(alu, ALU, 32);
    SUBCOMPONENT_NT(regs, RISCV_RegisterFile);

    tst_addOp() {
        // Connect objects

        connect(c4->out, alu->op1);
        connect(alu_ctrl->out, alu->ctrl);
        connect(regs->operands[0], alu->op2);
        connect(c1->out, regs->writeEnable);
        connect(c5->out, regs->writeRegister);
        connect(alu->out, regs->writeData);
        connect(c_instr->out, regs->instruction);
    }
};
}  // namespace ripes

TEST_CASE("Test architecture creation") {
    ripes::tst_addOp a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();

    const int n = 10;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    REQUIRE(a.regs->value(5) == 40);
}
