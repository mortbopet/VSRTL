#include "ripes_alu.h"
#include "ripes_constant.h"
#include "ripes_register.h"
#include "ripes_registerfile.h"

#include "RISC-V/riscv_registerfile.h"

#include "catch.hpp"

/**
 * @brief tst_adderAndReg
 * Small test connecting an ALU, a constant and a register to test clocking of simple circuits
 */
/*
namespace ripes {
class tst_registerFile : public Architecture<3> {
public:
    static constexpr int resReg = 5;

    tst_registerFile() : Architecture() {
        // clang-format off
        Component<
                bundle<
                    P("clk", 1),
                    P("sig", 1)
                >,
                bundle<
                    P("out", 1)
                >> a;
        // clang-format on

        // Create objects
        auto alu_ctrl = create<Constant<ALUctrlWidth(), ALU_OPCODE::ADD>>();
        auto c5 = create<Constant<32, resReg>>();
        auto c4 = create<Constant<32, 4>>();
        auto c1 = create<Constant<32, 1>>();
        auto c_instr = create<Constant<32, resReg << (7 + 5)>>();
        auto alu = create<ALU<32>>();

        auto rf = create<RISCV_RegisterFile>();

        // Connect objects

        alu->connect<0>(c4);
        alu->connect<1>(rf->getOperand<0>());
        alu->connectAdditional<0>(alu_ctrl);

        rf->connect<0>(c_instr);
        rf->connectAdditional<RISCV_RegisterFile::AdditionalInputs::writeRegister>(c5);
        rf->connectAdditional<RISCV_RegisterFile::AdditionalInputs::writeEnable>(c1);
        rf->connectAdditional<RISCV_RegisterFile::AdditionalInputs::writeData>(alu);
    }
};
}  // namespace ripes

TEST_CASE("Test architecture creation") {
    ripes::tst_registerFile a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();

    const int n = 10;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    // REQUIRE(static_cast<uint32_t>(*a.m_reg) == expectedValue);
}
*/
