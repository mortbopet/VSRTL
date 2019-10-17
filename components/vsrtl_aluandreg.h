#pragma once
#include "vsrtl_alu.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_register.h"

#include "vsrtl_alu.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_register.h"
namespace vsrtl {

class ALUAndReg : public Design {
public:
    ALUAndReg() : Design("ALU and Register") {
        // Connect objects
        c4->out >> alu->op1;
        reg->out >> alu->op2;
        alu_ctrl->out >> alu->ctrl;
        alu->out >> reg->in;
    }
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(alu_ctrl, Constant<ALU_OPCODE::width()>, ALU_OPCODE::ADD);
    SUBCOMPONENT(c4, Constant<32>, 4);
    SUBCOMPONENT(alu, ALU<32>);
    SUBCOMPONENT(reg, Register<32>);
};
}  // namespace vsrtl
