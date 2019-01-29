#ifndef VSRTL_ADDERANDREG_H
#define VSRTL_ADDERANDREG_H

#include "vsrtl_alu.h"
#include "vsrtl_architecture.h"
#include "vsrtl_constant.h"
#include "vsrtl_register.h"

namespace vsrtl {

class AdderAndReg : public Architecture {
public:
    AdderAndReg() : Architecture() {
        // Connect objects
        connectSignal(c4->value, alu->op1);
        connectSignal(reg->out, alu->op2);
        connectSignal(alu_ctrl->value, alu->ctrl);
        connectSignal(alu->out, reg->in);
    }
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(alu_ctrl, Constant, ALUctrlWidth(), ALU_OPCODE::SUB);
    SUBCOMPONENT(c4, Constant, 32, 4);
    SUBCOMPONENT(alu, ALU, 32);
    SUBCOMPONENT(reg, Register, 32);
};
}  // namespace vsrtl
#endif  // VSRTL_ADDERANDREG_H
