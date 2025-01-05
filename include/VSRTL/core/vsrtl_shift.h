#ifndef VSRTL_SHIFT_H
#define VSRTL_SHIFT_H

#include "vsrtl_component.h"

namespace vsrtl {
namespace core {

enum class ShiftType { sl, sra, srl };

template <unsigned int W>
class Shift : public Component {
public:
  Shift(const std::string &name, SimComponent *parent, ShiftType t,
        unsigned int shamt)
      : Component(name, parent) {
    out << [=] {
      if (t == ShiftType::sl) {
        return in.uValue() << shamt;
      } else if (t == ShiftType::sra) {
        return VT_U(in.sValue() >> shamt);
      } else if (t == ShiftType::srl) {
        return in.uValue() >> shamt;
      } else {
        throw std::runtime_error("Unknown shift type");
      }
    };
  }

  OUTPUTPORT(out, W);
  INPUTPORT(in, W);
};

} // namespace core
} // namespace vsrtl
#endif // VSRTL_SHIFT_H
