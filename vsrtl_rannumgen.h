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

        xOr3->out >> *mux->in[1];

        mux->out >> rngResReg->in;

        // Initialization selection. The RNG must be supplied with a seed value - in this case, select 'init'
        // constant as the input of the rng register in the first clock cycle, and the RNG circuit for all others.
        init->out >> *mux->in[0];
        orr->out >> selReg->in;
        selReg->out >> *orr->in[0];
        c1->out >> *orr->in[1];
        selReg->out >> mux->select;
    }
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(sh1, Shift, ShiftType::sl, 13, 32);
    SUBCOMPONENT(sh2, Shift, ShiftType::srl, 17, 32);
    SUBCOMPONENT(sh3, Shift, ShiftType::sl, 5, 32);
    SUBCOMPONENT(xOr1, Xor, 2, 32);
    SUBCOMPONENT(xOr2, Xor, 2, 32);
    SUBCOMPONENT(xOr3, Xor, 2, 32);
    SUBCOMPONENT(rngResReg, Register, 32);

    // Initialization objects for first clock cycle
    SUBCOMPONENT(mux, Multiplexer, 2, 32);
    SUBCOMPONENT(init, Constant, 0x13fb27a3, 32);
    SUBCOMPONENT(c1, Constant, 1, 32);
    SUBCOMPONENT(orr, Or, 2, 32);
    SUBCOMPONENT(selReg, Register, 32);
};
}  // namespace vsrtl

#endif  // VSRTL_RANNUMGEN_H
