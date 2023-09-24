#ifndef VSRTL_NBITADDER_H
#define VSRTL_NBITADDER_H

#include "vsrtl_collator.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_fulladder.h"

#include <string>

namespace vsrtl {
namespace core {

template <unsigned int width>
class Counter : public Design {
  static_assert((sizeof(VSRTL_VT_U) * CHAR_BIT) >= width,
                "Counter width greater than VSRTL valuetype width");

public:
  Counter() : Design(std::to_string(width) + " bit counter") {
    // Connect
    0 >> adders[0]->Cin;
    1 >> adders[0]->A;
    regs[0]->out >> adders[0]->B;
    regs[0]->out >> *value->in[0];
    adders[0]->S >> regs[0]->in;

    for (unsigned i = 1; i < width; i++) {
      adders[i - 1]->Cout >> adders[i]->Cin;
      regs[i]->out >> adders[i]->A;
      regs[i]->out >> *value->in[i];
      0 >> adders[i]->B;
      adders[i]->S >> regs[i]->in;
    }

    value->out >> outputReg->in;
  }

  SUBCOMPONENTS(adders, FullAdder, width);
  SUBCOMPONENTS(regs, Register<1>, width);
  SUBCOMPONENT(outputReg, Register<width>);
  SUBCOMPONENT(value, Collator<width>);
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_NBITADDER_H
