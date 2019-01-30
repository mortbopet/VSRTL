#ifndef VSRTL_ADDERANDREG_H
#define VSRTL_ADDERANDREG_H

#include "vsrtl_alu.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_register.h"

namespace vsrtl {

class AdderAndReg : public Design {
public:
    AdderAndReg() : Design() {
        // Connect objects
        c4->value >> alu->op1;
        reg->out >> alu->op2;
        alu_ctrl->value >> alu->ctrl;
        alu->out >> reg->in;
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
