#pragma once
#include "VSRTL/core/vsrtl_alu.h"
#include "VSRTL/core/vsrtl_constant.h"
#include "VSRTL/core/vsrtl_design.h"
#include "VSRTL/core/vsrtl_register.h"

#include "VSRTL/core/vsrtl_alu.h"
#include "VSRTL/core/vsrtl_constant.h"
#include "VSRTL/core/vsrtl_design.h"
#include "VSRTL/core/vsrtl_register.h"
namespace vsrtl {
namespace core {

class ALUAndReg : public Design {
public:
  ALUAndReg() : Design("ALU and Register") {
    // Connect objects
    4 >> alu->op1;
    reg->out >> alu->op2;
    (static_cast<size_t>(ALU_OPCODE::ADD)) >> alu->ctrl;
    alu->out >> reg->in;
  }
  static constexpr int m_cVal = 4;

  // Create objects
  SUBCOMPONENT(alu, ALU<32>);
  SUBCOMPONENT(reg, Register<32>);
};

} // namespace core
} // namespace vsrtl
