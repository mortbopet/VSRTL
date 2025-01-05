#ifndef VSRTL_RANNUMGEN_H
#define VSRTL_RANNUMGEN_H

#include "vsrtl_alu.h"
#include "vsrtl_comparator.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_logicgate.h"
#include "vsrtl_multiplexer.h"
#include "vsrtl_shift.h"

namespace vsrtl {
namespace core {

class RanNumGen : public Design {
public:
  RanNumGen() : Design("Random Number Generator") {
    // Connect objects
    rngResReg->out >> sh1->in;

    rngResReg->out >> *xOr1->in[0];
    sh1->out >> *xOr1->in[1];

    xOr1->out >> *xOr2->in[0];
    xOr1->out >> sh2->in;
    sh2->out >> *xOr2->in[1];

    xOr2->out >> *xOr3->in[0];
    xOr2->out >> sh3->in;
    sh3->out >> *xOr3->in[1];

    xOr3->out >> *mux->ins[1];

    mux->out >> rngResReg->in;

    // Initialization selection. The RNG must be supplied with a seed value - in
    // this case, select 'init' constant as the input of the rng register in the
    // first clock cycle, and the RNG circuit for all others.
    0x13fb27a3 >> *mux->ins[0];
    orr->out >> selReg->in;
    selReg->out >> *orr->in[0];
    1 >> *orr->in[1];
    selReg->out >> mux->select;
  }
  static constexpr int m_cVal = 4;

  // Create objects
  SUBCOMPONENT(sh1, Shift<32>, ShiftType::sl, 13);
  SUBCOMPONENT(sh2, Shift<32>, ShiftType::srl, 17);
  SUBCOMPONENT(sh3, Shift<32>, ShiftType::sl, 5);
  SUBCOMPONENT(xOr1, TYPE(Xor<32, 2>));
  SUBCOMPONENT(xOr2, TYPE(Xor<32, 2>));
  SUBCOMPONENT(xOr3, TYPE(Xor<32, 2>));
  SUBCOMPONENT(rngResReg, Register<32>);
  SUBCOMPONENT(mux, TYPE(Multiplexer<2, 32>));

  // Initialization objects for first clock cycle
  SUBCOMPONENT(orr, TYPE(Or<1, 2>));
  SUBCOMPONENT(selReg, Register<1>);
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_RANNUMGEN_H
