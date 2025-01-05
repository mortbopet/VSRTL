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
namespace core {

class ALUAndReg : public Design {
public:
  ALUAndReg() : Design("ALU and Register") {
    // Connect objects
    4 >> alu->op1;
    reg->out >> alu->op2;
    ALU_OPCODE::ADD >> alu->ctrl;
    alu->out >> reg->in;
  }
  static constexpr int m_cVal = 4;

  // Create objects
  SUBCOMPONENT(alu, ALU<32>);
  SUBCOMPONENT(reg, Register<32>);
};

} // namespace core
} // namespace vsrtl
