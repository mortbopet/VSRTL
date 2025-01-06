/*
Various hardware enums for Leros.
Dev note: enum's are used instead of enum class to have "easy" access to the
underlying enum index values, which is used when propagating enum values through
VSRTL.
the outer structs are used for namespacing the enums - required, since some enum
values alias (e.g. 'nop').
*/
#pragma once

namespace vsrtl {
namespace leros {

#define LEROS_INSTR_WIDTH 16
#define LEROS_REG_WIDTH 32

#define LEROS_REGS 256

struct alu_op {
  enum opcodes {
    nop,
    add,
    sub,
    shra,
    alu_and,
    alu_or,
    alu_xor,
    loadi,
    loadhi,
    loadh2i,
    loadh3i
  };
};

struct imm_op {
  enum opcodes {
    nop,
    shl1,
    shl2,
    branch,
    loadi,
    loadhi,
    loadh2i,
    loadh3i,
    jal
  };
};

struct mem_op {
  enum opcodes { nop, wr, rd };
};

struct acc_reg_src {
  enum opcodes { acc, alu, reg, dm };
};

struct alu_op1_op {
  enum opcodes { acc, pc, addr };
};

struct alu_op2_op {
  enum opcodes { unused, reg, imm };
};

struct access_size_op {
  enum opcodes { byte, half, word };
};

struct br_op {
  enum opcodes { nop, br, brz, brnz, brp, brn };
};

struct LerosInstr {
  enum opcodes {
    nop,
    add,
    addi,
    sub,
    subi,
    shra,
    load,
    loadi,
    andr,
    andi,
    orr,
    ori,
    xorr,
    xori,
    loadhi,
    loadh2i,
    loadh3i,
    store,
    ioout,
    ioin,
    jal,
    ldaddr,
    ldind,
    ldindb,
    ldindh,
    stind,
    stindb,
    stindh,
    br,
    brz,
    brnz,
    brp,
    brn,
    scall
  };
};

struct pc_reg_src {
  enum opcodes { alu, acc, pc2 };
};

struct addr_reg_src {
  enum opcodes { reg, addrreg };
};

struct reg_data_src {
  enum opcodes { alu, acc, nop };
};

} // namespace leros
} // namespace vsrtl
