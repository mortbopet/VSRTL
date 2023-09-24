#pragma once

#include "vsrtl_adder.h"
#include "vsrtl_constant.h"
#include "vsrtl_decollator.h"
#include "vsrtl_design.h"
#include "vsrtl_logicgate.h"
#include "vsrtl_multiplexer.h"
#include "vsrtl_register.h"

namespace vsrtl {
namespace core {

class XorNetwork : public Design {
public:
  static constexpr unsigned int rows = 100;
  static constexpr unsigned int cols = 50;

  XorNetwork() : Design("XOr Network") {
    // Driver setup
    0x1234abcd >> adder->op1;
    seedReg->out >> adder->op2;
    adder->out >> seedReg->in;
    adder->out >> decol->in;

    // Muxes, Registers & first XOR column setup
    for (size_t i = 0; i < rows; i++) {
      const size_t xor1 = i;
      const size_t xor2 = (i + rows / 2) % rows;
      *decol->out[i] >> *xors[xor1]->in[0];
      *decol->out[i] >> *xors[xor2]->in[1];
    }

    // XOR columns
    for (size_t i = 0; i <= cols - 2; i++) {
      for (size_t k = 0; k < rows; k++) {
        const size_t fromXor = i * rows + k;
        const size_t xor1 = fromXor + rows;
        size_t xor2 = (fromXor + rows + rows / 2);
        if (xor2 >= rows * (i + 2)) {
          xor2 -= rows;
        }
        xors[i * rows + k]->out >> *xors[xor1]->in[0];
        xors[i * rows + k]->out >> *xors[xor2]->in[1];
      }
    }
  }

  // Create objects
  SUBCOMPONENTS(xors, TYPE(Or<1, 2>), cols *rows);

  SUBCOMPONENT(seedReg, Register<rows>);
  SUBCOMPONENT(decol, Decollator<rows>);
  SUBCOMPONENT(adder, Adder<rows>);
};
} // namespace core
} // namespace vsrtl
