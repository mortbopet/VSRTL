#pragma once

#include "VSRTL/core/vsrtl_component.h"

#include "common.h"

namespace vsrtl {
using namespace core;

namespace leros {

class Branch : public Component {
public:
  Branch(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    do_branch << [=] {
      switch (op) {
      case br_op::br:
        return true;
      case br_op::brp:
        return acc.sValue() >= 0;
      case br_op::brn:
        return acc.sValue() < 0;
      case br_op::brz:
        return acc.sValue() == 0;
      case br_op::brnz:
        return acc.sValue() != 0;
      case br_op::nop:
      default:
        return false;
      }
    };

    pc_ctrl << [=] {
      switch (op) {
      case br_op::br:
      case br_op::brp:
      case br_op::brn:
      case br_op::brz:
      case br_op::brnz:
        return pc_reg_src::alu;
      default:
        break;
      }

      switch (instr_op) {
      case LerosInstr::jal:
        return pc_reg_src::acc;
      default:
        break;
      }

      return pc_reg_src::pc2;
    };
  }

  INPUTPORT(acc, LEROS_REG_WIDTH);
  INPUTPORT_ENUM(op, br_op);
  INPUTPORT_ENUM(instr_op, LerosInstr);

  OUTPUTPORT_ENUM(pc_ctrl, pc_reg_src);
  OUTPUTPORT(do_branch, 1);
};

} // namespace leros
} // namespace vsrtl
