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
    std::shared_ptr<Register<32>> m_reg;

    tst_adderAndReg() : Architecture() {
        // Create objects
        auto alu_ctrl = create<Constant<ALUctrlWidth(), ALU_OPCODE::ADD>>();
        auto c4 = create<Constant<32, m_cVal>>();
        auto alu = create<ALU<32>>();
        m_reg = create<Register<32>>();

        // Connect objects
        alu->connect<0>(c4);
        alu->connectAdditional<0>(alu_ctrl);
        alu->connect<1>(m_reg);
        m_reg->connect<0>(alu);
    }
};
}  // namespace ripes

TEST_CASE("Test architecture creation") {
    ripes::tst_adderAndReg a;

    // Verify that all instantiated objects in the circuit have been connected as they require
    a.verifyAndInitialize();

    const int n = 10;
    const int expectedValue = n * a.m_cVal;
    // Clock the circuit n times
    for (int i = 0; i < n; i++)
        a.clock();

    // We expect that m_cVal has been added to the register value n times
    REQUIRE(static_cast<uint32_t>(*a.m_reg) == expectedValue);
}
