#pragma once

#include "../interface/vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_enum.h"
#include "vsrtl_port.h"
#include <cstdint>

#include "../interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {
namespace core {

Enum(ALU_OPCODE, ADD, SUB, MUL, DIV, AND, OR, XOR, SL, SRA, SRL, LUI, LT, LTU,
     EQ);

template <unsigned int W>
class ALU : public Component {
public:
  SetGraphicsType(ALU);
  ALU(const std::string &name, SimComponent *parent) : Component(name, parent) {
    out << ([=] { return calculateOutput(); });
  }

  void propagate() { calculateOutput(); }

  INPUTPORT(op1, W);
  INPUTPORT(op2, W);
  INPUTPORT(ctrl, ALU_OPCODE::width());

  OUTPUTPORT(out, W);

private:
  VSRTL_VT_U calculateOutput() {
    const auto uop1 = op1.uValue();
    const auto uop2 = op2.uValue();
    const auto _op1 = op1.sValue();
    const auto _op2 = op2.sValue();
    Switch(ctrl, ALU_OPCODE) {
    case ALU_OPCODE::ADD:
      return uop1 + uop2;
    case ALU_OPCODE::SUB:
      return uop1 - uop2;
    case ALU_OPCODE::MUL:
      return uop1 * uop2;
    case ALU_OPCODE::DIV:
      return uop1 / uop2;
    case ALU_OPCODE::AND:
      return uop1 & uop2;
    case ALU_OPCODE::OR:
      return uop1 | uop2;
    case ALU_OPCODE::XOR:
      return uop1 ^ uop2;
    case ALU_OPCODE::SL:
      return uop1 << uop2;
    case ALU_OPCODE::SRA:
      return _op1 >> uop2;
    case ALU_OPCODE::SRL:
      return uop1 >> uop2;
    case ALU_OPCODE::LUI:
      return uop2;
    case ALU_OPCODE::LT:
      return _op1 < _op2 ? 1 : 0;
    case ALU_OPCODE::LTU:
      return uop1 < uop2 ? 1 : 0;
    default:
      throw std::runtime_error("Invalid ALU opcode");
    }
  }
};
} // namespace core
} // namespace vsrtl
