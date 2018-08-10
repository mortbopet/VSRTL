#include "../core/ripes_alu.h"
#include "ripes_architecture.h"
#include "ripes_constant.h"
#include "ripes_register.h"

#include "catch.hpp"

#include <iostream>

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
        connectSignal(c4->out, alu->op1);
        connectSignal(reg->out, alu->op2);
        connectSignal(alu_ctrl->out, alu->ctrl);
        connectSignal(alu->out, reg->in);
    }
};
}  // namespace ripes

TEST_CASE("Test adder and reg") {
    ripes::tst_adderAndReg a;

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
    REQUIRE(a.reg->out->value<uint32_t>() == expectedValue);
}
