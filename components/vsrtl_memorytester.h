#pragma once

#include "vsrtl_adder.h"
#include "vsrtl_comparator.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_memory.h"
#include "vsrtl_multiplexer.h"
#include "vsrtl_register.h"

namespace vsrtl {

/**
 * @brief The MemoryTester design
 * A memory is instantiated wherein the following operation is executed:
 *      mem[i] = mem[i-1] + 1;
 */
class MemoryTester : public Design {
public:
    MemoryTester() : Design("Registerfile Tester") {
        idx_reg->out >> mem->addr;
        mem->data_out >> acc_reg->in;
        inc_adder->out >> mem->data_in;
        c1->out >> idx_adder->op1;
        idx_reg->out >> idx_adder->op2;
        c1L->out >> inc_adder->op1;
        acc_reg->out >> inc_adder->op2;

        wr_en_reg->out >> idx_next_mux->select;
        idx_adder->out >> *idx_next_mux->ins[0];
        idx_reg->out >> *idx_next_mux->ins[1];

        idx_reg->out >> comp->op1;
        cIdxMax->out >> comp->op2;
        idx_next_mux->out >> idx_reg->in;

        // Write/Read state
        wr_en_mux->out >> wr_en_reg->in;
        wr_en_reg->out >> wr_en_mux->select;
        wr_en_reg->out >> mem->wr_en;
        c0B->out >> *wr_en_mux->ins[1];
        c1B->out >> *wr_en_mux->ins[0];
    }
    static constexpr unsigned int regSize = 32;

    // Create objects
    SUBCOMPONENT(mem, TYPE(Memory<regSize, regSize>));

    SUBCOMPONENT(idx_adder, Adder<regSize>);
    SUBCOMPONENT(inc_adder, Adder<regSize>);
    SUBCOMPONENT(c1, Constant<regSize>, 1);
    SUBCOMPONENT(c1L, Constant<regSize>, 1);
    SUBCOMPONENT(c1B, Constant<1>, 1);
    SUBCOMPONENT(c0B, Constant<1>, 0);
    SUBCOMPONENT(cIdxMax, Constant<regSize>, regSize - 1);
    SUBCOMPONENT(wr_en_mux, TYPE(Multiplexer<2, 1>));
    SUBCOMPONENT(idx_next_mux, TYPE(Multiplexer<2, regSize>));

    SUBCOMPONENT(comp, Eq<regSize>);

    SUBCOMPONENT(wr_en_reg, Register<1>);
    SUBCOMPONENT(idx_reg, Register<regSize>);
    SUBCOMPONENT(acc_reg, Register<regSize>);
};
}  // namespace vsrtl
