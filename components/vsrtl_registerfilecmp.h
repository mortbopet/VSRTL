#ifndef VSRTL_REGISTERFILECMP_H
#define VSRTL_REGISTERFILECMP_H

#include "vsrtl_adder.h"
#include "vsrtl_design.h"
#include "vsrtl_registerfile.h"

namespace vsrtl {
namespace core {

class RegisterFileTester : public Design {
public:
  static constexpr unsigned int regFiles = 1;

  RegisterFileTester() : Design("Registerfile Tester") {
    for (unsigned i = 0; i < regFiles; i++) {
      idx_reg->out >> regs[i]->rd_idx;
      idx_adder->out >> regs[i]->wr_idx;
      reg_adder->out >> regs[i]->wr_data;
      1 >> regs[i]->wr_en;

      if (i == 0) {
        idx_adder->out >> idx_reg->in;
        1 >> idx_adder->op1;
        idx_reg->out >> idx_adder->op2;
        1 >> reg_adder->op1;
        regs[i]->rd_data >> reg_adder->op2;
      }
    }
  }
  static constexpr unsigned int regSize = 32;

  // Create objects
  SUBCOMPONENTS(regs, TYPE(RegisterFile<regSize, regSize>), regFiles);

  SUBCOMPONENT(idx_adder, Adder<ceillog2(regSize)>);
  SUBCOMPONENT(reg_adder, Adder<regSize>);
  SUBCOMPONENT(idx_reg, Register<ceillog2(regSize)>);
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_REGISTERFILECMP_H
