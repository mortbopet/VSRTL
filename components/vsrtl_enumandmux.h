#pragma once

#include "vsrtl_adder.h"
#include "vsrtl_comparator.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_enum.h"
#include "vsrtl_multiplexer.h"

namespace vsrtl {

Enum(TestEnum, A, B, C, D, E, F);

class EnumAndMux : public Design {
public:
    EnumAndMux() : Design("Enum and Multiplexer") {
        // Connect objects
        // c4->out >> adder->op1;s
        cA->out >> mux->get(TestEnum::A);
        cB->out >> mux->get(TestEnum::B);
        cE->out >> mux->get(TestEnum::E);
        cDefault->out >> mux->others();

        reg->out >> mux->select;

        c1->out >> adder->op1;
        reg->out >> adder->op2;

        cMax->out >> cmp->op1;
        reg->out >> cmp->op2;

        // Register next-state input mux
        cmp->out >> regIn_mux->select;
        c0->out >> *regIn_mux->ins[1];
        adder->out >> *regIn_mux->ins[0];
        regIn_mux->out >> reg->in;
    }
    static constexpr int width = 32;

    // Create objects
    SUBCOMPONENT(mux, TYPE(EnumMultiplexer<TestEnum, width>));
    SUBCOMPONENT(adder, Adder<TestEnum::width()>);
    SUBCOMPONENT(c0, Constant<TestEnum::width()>, 0);
    SUBCOMPONENT(c1, Constant<TestEnum::width()>, 1);
    SUBCOMPONENT(cA, Constant<width>, 1);
    SUBCOMPONENT(cB, Constant<width>, 2);
    SUBCOMPONENT(cE, Constant<width>, 3);
    SUBCOMPONENT(cDefault, Constant<width>, 0XDEADBEEF);
    SUBCOMPONENT(reg, Register<TestEnum::width()>);
    SUBCOMPONENT(regIn_mux, TYPE(Multiplexer<2, TestEnum::width()>));
    SUBCOMPONENT(cmp, Eq<TestEnum::width()>);
    SUBCOMPONENT(cMax, Constant<TestEnum::width()>, TestEnum::count() - 1);
};
}  // namespace vsrtl
