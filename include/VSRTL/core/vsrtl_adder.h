#pragma once

#include "VSRTL/core/vsrtl_component.h"
#include "VSRTL/interface/vsrtl_defines.h"

#include "VSRTL/interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {
namespace core {

template <unsigned int W>
class Adder : public Component {
public:
  SetGraphicsType(Adder);
  Adder(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    out << [this] { return op1.sValue() + op2.sValue(); };
  }

  INPUTPORT(op1, W);
  INPUTPORT(op2, W);
  OUTPUTPORT(out, W);
};
} // namespace core
} // namespace vsrtl
