#pragma once

#include "vsrtl_adder.h"
#include "vsrtl_comparator.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_enum.h"
#include "vsrtl_multiplexer.h"

namespace vsrtl {
namespace core {

Enum(TestEnum, A, B, C, D, E, F);

class EnumAndMux : public Design {
public:
  EnumAndMux() : Design("Enum and Multiplexer") {
    // Connect objects
    1 >> mux->get(TestEnum::A);
    2 >> mux->get(TestEnum::B);
    3 >> mux->get(TestEnum::E);
    0xDEADBEEF >> mux->others();

    reg->out >> mux->select;

    1 >> adder->op1;
    reg->out >> adder->op2;

    (TestEnum::_size() - 1) >> cmp->op1;
    reg->out >> cmp->op2;

    // Register next-state input mux
    cmp->out >> regIn_mux->select;
    0 >> *regIn_mux->ins[1];
    adder->out >> *regIn_mux->ins[0];
    regIn_mux->out >> reg->in;
  }
  static constexpr int width = 32;

  // Create objects
  SUBCOMPONENT(mux, TYPE(EnumMultiplexer<TestEnum, width>));
  SUBCOMPONENT(adder, Adder<TestEnum::width()>);
  SUBCOMPONENT(reg, Register<TestEnum::width()>);
  SUBCOMPONENT(regIn_mux, TYPE(Multiplexer<2, TestEnum::width()>));
  SUBCOMPONENT(cmp, Eq<TestEnum::width()>);
};

} // namespace core
} // namespace vsrtl
