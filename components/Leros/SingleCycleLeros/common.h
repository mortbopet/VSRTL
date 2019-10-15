#pragma once

#include "vsrtl_enum.h"

namespace vsrtl {
namespace leros {

#define LEROS_INSTR_WIDTH 16
#define LEROS_REG_WIDTH 32

#define LEROS_REGS 256

Enum(alu_op, nop, add, sub, shra, alu_and, alu_or, alu_xor, loadi, loadhi, loadh2i, loadh3i);
Enum(imm_op, nop, shl1, shl2, branch, loadi, loadhi, loadh2i, loadh3i, jal);
Enum(mem_op, nop, wr, rd);
Enum(acc_reg_src, acc, alu, reg, dm);
Enum(alu_op1_op, acc, pc, addr);
Enum(alu_op2_op, unused, reg, imm);
Enum(access_size_op, byte, half, word);
Enum(br_op, nop, br, brz, brnz, brp, brn);
Enum(LerosInstr, nop, add, addi, sub, subi, shra, load, loadi, andr, andi, orr, ori, xorr, xori, loadhi, loadh2i,
     loadh3i, store, ioout, ioin, jal, ldaddr, ldind, ldindb, ldindh, stind, stindb, stindh, br, brz, brnz, brp, brn,
     scall);
Enum(pc_reg_src, alu, acc, pc2);
Enum(addr_reg_src, reg, addrreg);
Enum(reg_data_src, alu, acc, nop);

}  // namespace leros
}  // namespace vsrtl
