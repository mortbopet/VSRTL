#ifndef VSRTL_NBITADDER_H
#define VSRTL_NBITADDER_H

#include "vsrtl_collator.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_fulladder.h"

#include <string>

namespace vsrtl {

template <unsigned int width>
class Counter : public Design {
public:
    Counter() : Design(std::to_string(width) + " bit counter") {
        for (int i = 0; i < width; i++) {
            adders.push_back(create_component<FullAdder>(this, "adder_" + std::to_string(i)));
            regs.push_back(create_component<Register>(this, "reg_" + std::to_string(i), 1));
        }

        // Connect
        c0->out >> adders[0]->Cin;
        c1->out >> adders[0]->A;
        regs[0]->out >> adders[0]->B;
        regs[0]->out >> *value->in[0];
        adders[0]->S >> regs[0]->in;

        for (int i = 1; i < width; i++) {
            adders[i - 1]->Cout >> adders[i]->Cin;
            regs[i]->out >> adders[i]->A;
            regs[i]->out >> *value->in[i];
            c0->out >> adders[i]->B;
            adders[i]->S >> regs[i]->in;
        }
    }

    std::vector<FullAdder*> adders;
    std::vector<Register*> regs;

    SUBCOMPONENT(value, Collator, width);

    SUBCOMPONENT(c0, Constant, 0, 1);
    SUBCOMPONENT(c1, Constant, 1, 1);
};
}  // namespace vsrtl

#endif  // VSRTL_NBITADDER_H
