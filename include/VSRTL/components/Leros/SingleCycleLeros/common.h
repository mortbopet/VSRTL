/*
Various hardware enums for Leros.
Dev note: enum's are used instead of enum class class to have "easy" access to
the underlying enum class index values, which is used when propagating enum
class values through VSRTL. the outer structs are used for namespacing the enums
- required, since some enum values alias (e.g. 'nop').
*/
#pragma once

namespace vsrtl {
namespace leros {

#define LEROS_INSTR_WIDTH 16
#define LEROS_REG_WIDTH 32

#define LEROS_REGS 256

enum class alu_op {
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

enum class imm_op {
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

enum class mem_op { nop, wr, rd };
enum class acc_reg_src { acc, alu, reg, dm };
enum class alu_op1_op { acc, pc, addr };
enum class alu_op2_op { unused, reg, imm };
enum class access_size_op { byte, half, word };
enum class br_op { nop, br, brz, brnz, brp, brn };
enum class LerosInstr {
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

enum class pc_reg_src { alu, acc, pc2 };
enum class addr_reg_src { reg, addrreg };
enum class reg_data_src { alu, acc, nop };

} // namespace leros
} // namespace vsrtl
