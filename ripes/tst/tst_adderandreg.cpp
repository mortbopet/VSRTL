#include "ripes_alu.h"
#include "ripes_architecture.h"
#include "ripes_constant.h"
#include "ripes_register.h"

#include "catch.hpp"

namespace ripes {
/**
 * @brief tst_adderAndReg
 * Small test connecting an ALU, a constant and a register to test clocking of simple circuits
 */
class tst_adderAndReg : public Architecture<3> {
public:
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(alu_ctrl, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);
    SUBCOMPONENT(c4, Constant, 32, 4);
    SUBCOMPONENT(alu, ALU, 32);
    SUBCOMPONENT(reg, Register, 32);

    tst_adderAndReg() : Architecture() {
        // Connect objects
        c4->m_output >> alu->m_op1;
        reg >> alu->m_op2;
        alu_ctrl >> alu->m_ctrl;
        alu->m_output >> reg;
    }
};
}  // namespace ripes

TEST_CASE("Test adder and reg") {
    ripes::tst_adderAndReg a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();

    const int n = 10;
    const int expectedValue = n * a.m_cVal;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    REQUIRE(a.reg->m_output->value<uint32_t>() == expectedValue);
}
