#ifndef RIPES_ADDERANDREG_H
#define RIPES_ADDERANDREG_H

#include "ripes_alu.h"
#include "ripes_architecture.h"
#include "ripes_constant.h"
#include "ripes_register.h"

namespace ripes {

class AdderAndReg : public Architecture {
public:
    AdderAndReg() : Architecture() {
        // Connect objects
        connectSignal(c4->out, alu->op1);
        connectSignal(reg->out, alu->op2);
        connectSignal(alu_ctrl->out, alu->ctrl);
        connectSignal(alu->out, reg->in);
    }
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(alu_ctrl, Constant, ALUctrlWidth(), ALU_OPCODE::ADD);
    SUBCOMPONENT(c4, Constant, 32, 4);
    SUBCOMPONENT(alu, ALU, 32);
    SUBCOMPONENT(reg, Register, 32);
};
}  // namespace ripes
#endif  // RIPES_ADDERANDREG_H
