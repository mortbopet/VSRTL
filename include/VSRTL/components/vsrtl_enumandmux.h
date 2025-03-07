#pragma once

#include "VSRTL/core/vsrtl_adder.h"
#include "VSRTL/core/vsrtl_comparator.h"
#include "VSRTL/core/vsrtl_constant.h"
#include "VSRTL/core/vsrtl_design.h"

#include "VSRTL/core/vsrtl_multiplexer.h"

namespace vsrtl {
namespace core {

enum TestEnum { A, B, C, D, E, F };

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

    (magic_enum::enum_count<TestEnum>() - 1) >> cmp->op1;
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
  SUBCOMPONENT(adder, Adder<enumBitWidth<TestEnum>()>);
  SUBCOMPONENT(reg, Register<enumBitWidth<TestEnum>()>);
  SUBCOMPONENT(regIn_mux, TYPE(Multiplexer<2, enumBitWidth<TestEnum>()>));
  SUBCOMPONENT(cmp, Eq<enumBitWidth<TestEnum>()>);
};

} // namespace core
} // namespace vsrtl
