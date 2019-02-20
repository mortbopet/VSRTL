#ifndef VSRTL_NBITADDER_H
#define VSRTL_NBITADDER_H

#include "vsrtl_fulladder.h"
#include "vsrtl_design.h"
#include "vsrtl_constant.h"

#include <string>

namespace vsrtl {


constexpr int size = 8;

class NBitAdder : public Design {
public:
    NBitAdder() : Design("N Bit Adder"){
        for(int i = 0 ; i < size; i++){
            adders.push_back(create_component<FullAdder>(this, "adder_" + std::to_string(i)));
            regs.push_back(create_component<Register>(this, "reg_" + std::to_string(i), 1));
        }

        // Connect
        c0->out >> adders[0]->Cin;
        c1->out >> adders[0]->A;
        regs[0]->out >> adders[0]->B;
        adders[0]->S >> regs[0]->in;

        for(int i = 1; i < size; i++){
            adders[i-1]->Cout >> adders[i]->Cin;
            regs[i]->out >> adders[i]->A;
            c0->out >> adders[i]->B;
            adders[i]->S >> regs[i]->in;
        }


    }

    std::vector<FullAdder*> adders;
    std::vector<Register*> regs;

    SUBCOMPONENT(c0, Constant, 0, 1);
    SUBCOMPONENT(c1, Constant, 1, 1);

};
}

#endif // VSRTL_NBITADDER_H
