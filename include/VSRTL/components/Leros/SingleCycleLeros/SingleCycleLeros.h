#ifndef VSRTL_SINGLECYCLELEROS_H
#define VSRTL_SINGLECYCLELEROS_H

#include "vsrtl_adder.h"
#include "vsrtl_comparator.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_memory.h"
#include "vsrtl_multiplexer.h"
#include "vsrtl_register.h"

#include "alu.h"
#include "branch.h"
#include "common.h"
#include "control.h"
#include "decode.h"
#include "immediate.h"

namespace vsrtl {
using namespace core;

namespace leros {

class SingleCycleLeros : public Design {
public:
    SingleCycleLeros() : Design("Single cycle Leros processor") {
        // -----------------------------------------------------------------------
        // Decode
        decode_comp->op >> ctrl_comp->instr_op;
        instr_mem->data_out >> decode_comp->instr;
        instr_mem->setMemory(m_memory);

        // -----------------------------------------------------------------------
        // Control signals
        ctrl_comp->alu_op1_ctrl >> alu_op1_mux->select;
        ctrl_comp->alu_op2_ctrl >> alu_op2_mux->select;
        ctrl_comp->alu_ctrl >> alu_comp->ctrl;

        // -----------------------------------------------------------------------
        // Immediate
        ctrl_comp->imm_ctrl >> imm_comp->ctrl;
        instr_mem->data_out >> imm_comp->instr;

        // -----------------------------------------------------------------------
        // Branch
        acc_reg->out >> br_comp->acc;
        ctrl_comp->br_ctrl >> br_comp->op;
        decode_comp->op >> br_comp->instr_op;

        // -----------------------------------------------------------------------
        // ALU
        alu_op1_mux->out >> alu_comp->op1;
        pc_reg->out >> alu_op1_mux->get(alu_op1_op::pc);
        acc_reg->out >> alu_op1_mux->get(alu_op1_op::acc);
        addr_reg->out >> alu_op1_mux->get(alu_op1_op::addr);

        alu_op2_mux->out >> alu_comp->op2;
        regs->data_out >> alu_op2_mux->get(alu_op2_op::reg);
        imm_comp->imm >> alu_op2_mux->get(alu_op2_op::imm);
        0 >> alu_op2_mux->get(alu_op2_op::unused);

        // -----------------------------------------------------------------------
        // Data memory
        ctrl_comp->dm_op >> mem_out_data_mux->select;
        acc_reg->out >> mem_out_data_mux->get(mem_op::wr);
        0 >> mem_out_data_mux->others();
        ceillog2(LEROS_REG_WIDTH / 8) >> data_mem->wr_width;

        ctrl_comp->dm_op >> mem_out_addr_mux->select;
        alu_comp->res >> mem_out_addr_mux->get(mem_op::wr);
        alu_comp->res >> mem_out_addr_mux->get(mem_op::rd);
        0 >> mem_out_addr_mux->others();

        data_mem->setMemory(m_memory);

        // -----------------------------------------------------------------------
        // Set write-enable of DM to high when the data operation is equal to wr
        mem_out_data_mux->out >> data_mem->data_in;
        mem_out_addr_mux->out >> data_mem->addr;
        dm_wr_en->out >> data_mem->wr_en;
        ctrl_comp->dm_op >> dm_wr_en->op1;
        mem_op::wr >> dm_wr_en->op2;

        // -----------------------------------------------------------------------
        // Registers
        decode_comp->lowByte >> regs->addr;
        ceillog2(LEROS_REG_WIDTH / 8) >> regs->wr_width;

        alu_comp->res >> reg_out_data_mux->get(reg_data_src::alu);
        acc_reg->out >> reg_out_data_mux->get(reg_data_src::acc);
        0 >> reg_out_data_mux->others();
        ctrl_comp->reg_data_src_ctrl >> reg_out_data_mux->select;
        reg_out_data_mux->out >> regs->data_in;

        reg_wr_en->out >> regs->wr_en;
        ctrl_comp->reg_op >> reg_wr_en->op1;
        mem_op::wr >> reg_wr_en->op2;

        regs->setMemory(m_regMemory);

        // -----------------------------------------------------------------------
        // Instruction memory
        pc_reg->out >> instr_mem->addr;

        // -----------------------------------------------------------------------
        // Next-state Program counter register
        pc_reg_next_mux->out >> pc_reg->in;
        br_comp->pc_ctrl >> pc_reg_next_mux->select;
        pcadd2->out >> pc_reg_next_mux->get(pc_reg_src::pc2);
        acc_reg->out >> pc_reg_next_mux->get(pc_reg_src::acc);
        alu_comp->res >> pc_reg_next_mux->get(pc_reg_src::alu);

        2 >> pcadd2->op1;
        pc_reg->out >> pcadd2->op2;

        // -----------------------------------------------------------------------
        // Next-state address register
        ctrl_comp->addr_reg_src_ctrl >> addr_reg_next_mux->select;
        addr_reg_next_mux->out >> addr_reg->in;
        regs->data_out >> addr_reg_next_mux->get(addr_reg_src::reg);
        addr_reg->out >> addr_reg_next_mux->get(addr_reg_src::addrreg);

        // -----------------------------------------------------------------------
        // Next-state accumulator register
        ctrl_comp->acc_reg_src_ctrl >> acc_reg_next_mux->select;
        alu_comp->res >> acc_reg_next_mux->get(acc_reg_src::alu);
        regs->data_out >> acc_reg_next_mux->get(acc_reg_src::reg);
        data_mem->data_out >> acc_reg_next_mux->get(acc_reg_src::dm);
        acc_reg->out >> acc_reg_next_mux->get(acc_reg_src::acc);
        acc_reg_next_mux->out >> acc_reg->in;
    }

    // Entities
    SUBCOMPONENT(imm_comp, Immediate);
    SUBCOMPONENT(br_comp, Branch);
    SUBCOMPONENT(ctrl_comp, Control);
    SUBCOMPONENT(decode_comp, Decode);
    SUBCOMPONENT(alu_comp, ALU);
    SUBCOMPONENT(pcadd2, Adder<LEROS_REG_WIDTH>);

    // Registers
    SUBCOMPONENT(acc_reg, Register<LEROS_REG_WIDTH>);
    SUBCOMPONENT(addr_reg, Register<LEROS_REG_WIDTH>);
    SUBCOMPONENT(pc_reg, Register<LEROS_REG_WIDTH>);

    // Memories
    SUBCOMPONENT(instr_mem, TYPE(ROM<LEROS_REG_WIDTH, LEROS_INSTR_WIDTH>));
    SUBCOMPONENT(data_mem, TYPE(MemoryAsyncRd<LEROS_REG_WIDTH, LEROS_REG_WIDTH>));
    SUBCOMPONENT(regs, TYPE(MemoryAsyncRd<ceillog2(LEROS_REGS), LEROS_REG_WIDTH>));

    // Multiplexers
    SUBCOMPONENT(reg_out_data_mux, TYPE(EnumMultiplexer<reg_data_src, LEROS_REG_WIDTH>));
    SUBCOMPONENT(mem_out_data_mux, TYPE(EnumMultiplexer<mem_op, LEROS_REG_WIDTH>));
    SUBCOMPONENT(mem_out_addr_mux, TYPE(EnumMultiplexer<mem_op, LEROS_REG_WIDTH>));
    SUBCOMPONENT(alu_op1_mux, TYPE(EnumMultiplexer<alu_op1_op, LEROS_REG_WIDTH>));
    SUBCOMPONENT(alu_op2_mux, TYPE(EnumMultiplexer<alu_op2_op, LEROS_REG_WIDTH>));
    SUBCOMPONENT(pc_reg_next_mux, TYPE(EnumMultiplexer<pc_reg_src, LEROS_REG_WIDTH>));
    SUBCOMPONENT(acc_reg_next_mux, TYPE(EnumMultiplexer<acc_reg_src, LEROS_REG_WIDTH>));
    SUBCOMPONENT(addr_reg_next_mux, TYPE(EnumMultiplexer<addr_reg_src, LEROS_REG_WIDTH>));

    // Comparators
    SUBCOMPONENT(dm_wr_en, Eq<mem_op::width()>);
    SUBCOMPONENT(reg_wr_en, Eq<mem_op::width()>);

    ADDRESSSPACEMM(m_memory);
    ADDRESSSPACE(m_regMemory);
};

}  // namespace leros
}  // namespace vsrtl
#endif  // VSRTL_SINGLECYCLELEROS_H
