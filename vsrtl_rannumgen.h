#ifndef VSRTL_RANNUMGEN_H
#define VSRTL_RANNUMGEN_H

#include "vsrtl_design.h"
#include "vsrtl_constant.h"
#include "vsrtl_logicgate.h"
#include "vsrtl_shift.h"
#include "vsrtl_multiplexer.h"
#include "vsrtl_alu.h"
#include "vsrtl_comparator.h"

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

        xOr3->out >> *mux->in[0];

        mux->out >> rngResReg->in;

        // Counter circuit
        add1->out >> c_reg->in;

        c_reg->out >> add1->op1;
        c1->value >> add1->op2;
        addctrl->value >> add1->ctrl;

        init->value >> *mux->in[1];

        // Comparator
        eq->out >> mux->select;
        c0->value >> eq->op1;
        c_reg->out >> eq->op2;
    }
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(sh1, Shift, 32, 13, ShiftType::sl);
    SUBCOMPONENT(sh2, Shift, 32, 17, ShiftType::srl);
    SUBCOMPONENT(sh3, Shift, 32, 5, ShiftType::sl);
    SUBCOMPONENT(xOr1, Xor, 32, 2);
    SUBCOMPONENT(xOr2, Xor, 32, 2);
    SUBCOMPONENT(xOr3, Xor, 32, 2);
    SUBCOMPONENT(rngResReg, Register, 32);

    SUBCOMPONENT(mux, Multiplexer, 2, 32);
    SUBCOMPONENT(init, Constant, 32, 0x13fb27a3);
    SUBCOMPONENT(c1, Constant, 32, 1);
    SUBCOMPONENT(c0, Constant, 32, 0);
    SUBCOMPONENT(addctrl, Constant, 32, 0);
    SUBCOMPONENT(add1, ALU, 32);
    SUBCOMPONENT(c_reg, Register, 32);
    SUBCOMPONENT(eq, Eq, 32);

};
}  // namespace vsrtl

#endif // VSRTL_RANNUMGEN_H
