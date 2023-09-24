#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include "vsrtl_comparator.h"
#include "vsrtl_component.h"
#include "vsrtl_constant.h"
#include "vsrtl_defines.h"
#include "vsrtl_multiplexer.h"
#include "vsrtl_port.h"
#include "vsrtl_register.h"

#include <memory>

namespace vsrtl {
namespace core {

template <int W, int N>
class RegisterFile : public Component {
public:
  RegisterFile(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    // Connect write (source) muxes
    for (int i = 0; i < N; i++) {
      src_muxes[i]->out >> regs[i]->in;
      wr_data >> *src_muxes[i]->ins[1];
      regs[i]->out >> *src_muxes[i]->ins[0];
      wr_idx >> cmps[i]->op1;
      i >> cmps[i]->op2;
      cmps[i]->out >> src_muxes[i]->select;
    }

    // Connect read (out) mux
    for (int i = 0; i < N; i++) {
      regs[i]->out >> *out_mux->ins[i];
    }
    rd_idx >> out_mux->select;
    out_mux->out >> rd_data;
  }

  INPUTPORT(wr_idx, ceillog2(N));
  INPUTPORT(wr_data, W);
  INPUTPORT(wr_en, 1);
  INPUTPORT(rd_idx, ceillog2(N));
  OUTPUTPORT(rd_data, W);

  SUBCOMPONENTS(regs, Register<W>, N);
  SUBCOMPONENTS(src_muxes, TYPE(Multiplexer<2, W>), N);
  SUBCOMPONENTS(cmps, Eq<ceillog2(N)>, N);
  SUBCOMPONENT(out_mux, TYPE(Multiplexer<N, W>));
};

} // namespace core
} // namespace vsrtl

#endif // REGISTERFILE_H
