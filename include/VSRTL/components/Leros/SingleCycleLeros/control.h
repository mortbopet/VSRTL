#pragma once
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_register.h"

#include "common.h"

namespace vsrtl {
using namespace core;

namespace leros {

class Control : public Component {
public:
    Control(const std::string& name, SimComponent* parent) : Component(name, parent) {
        alu_ctrl << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::add:    case LerosInstr::addi:   case LerosInstr::br:     case LerosInstr::brz:
                case LerosInstr::brnz:   case LerosInstr::brp:    case LerosInstr::brn:    case LerosInstr::jal:
                case LerosInstr::ldind:  case LerosInstr::ldindb: case LerosInstr::ldindh: case LerosInstr::stind:
                case LerosInstr::stindb: case LerosInstr::stindh:
                    return alu_op::add;
                case LerosInstr::sub:    case LerosInstr::subi:
                    return alu_op::sub;
                case LerosInstr::shra:
                    return alu_op::shra;
                case LerosInstr::andi:   case LerosInstr::andr:
                    return alu_op::alu_and;
                case LerosInstr::ori:    case LerosInstr::orr:
                    return alu_op::alu_or;
                case LerosInstr::xori:   case LerosInstr::xorr:
                    return alu_op::alu_xor;
                case LerosInstr::loadi:
                    return alu_op::loadi;
                case LerosInstr::loadhi:
                    return alu_op::loadhi;
                case LerosInstr::loadh2i:
                    return alu_op::loadh2i;
                case LerosInstr::loadh3i:
                    return alu_op::loadh3i;
                default:
                    return alu_op::nop;
            }
            // clang-format on
        };

        acc_reg_src_ctrl << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::add:    case LerosInstr::addi:   case LerosInstr::sub:    case LerosInstr::subi:
                case LerosInstr::shra:   case LerosInstr::andi:   case LerosInstr::andr:   case LerosInstr::xori:
                case LerosInstr::xorr:   case LerosInstr::ori:    case LerosInstr::orr:    case LerosInstr::loadi:
                case LerosInstr::loadhi: case LerosInstr::loadh2i:case LerosInstr::loadh3i:
                    return acc_reg_src::alu;
                case LerosInstr::ldind: case LerosInstr::ldindb: case LerosInstr::ldindh:
                    return acc_reg_src::dm;
                case LerosInstr::load:
                    return acc_reg_src::reg;
                default:
                    return acc_reg_src::acc;

            }
            // clang-format on
        };

        dm_op << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::ldind:  case LerosInstr::ldindb: case LerosInstr::ldindh:
                    return mem_op::rd;
                case LerosInstr::stind: case LerosInstr::stindb: case LerosInstr::stindh:
                    return mem_op::wr;
                default:
                    return mem_op::nop;
            }
            // clang-format on
        };

        dm_access_size << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::ldindb: case LerosInstr::stindb:
                    return access_size_op::byte;
                case LerosInstr::ldindh: case LerosInstr::stindh:
                    return access_size_op::half;
                case LerosInstr::ldind: case LerosInstr::stind:
                    return access_size_op::word;
                default:
                    return access_size_op::byte;
            }
            // clang-format on
        };

        reg_op << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::add: case LerosInstr::sub: case LerosInstr::andr:
                case LerosInstr::orr: case LerosInstr::xorr: case LerosInstr::load: case LerosInstr::ldaddr:
                    return mem_op::rd;
                case LerosInstr::store: case LerosInstr::jal:
                    return  mem_op::wr;
                default:
                    return mem_op::nop;
            }
            // clang-format on
        };

        imm_ctrl << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::addi: case LerosInstr::subi: case LerosInstr::andi: case LerosInstr::ori:
                case LerosInstr::xori: case LerosInstr::loadi:
                    return imm_op::loadi;
                case LerosInstr::loadhi:
                    return imm_op::loadhi;
                case LerosInstr::loadh2i:
                    return imm_op::loadh2i;
                case LerosInstr::loadh3i:
                    return imm_op::loadh3i;
                case LerosInstr::br:     case LerosInstr::brz:    case LerosInstr::brnz:   case LerosInstr::brp:
                case LerosInstr::brn:
                    return imm_op::branch;
                case LerosInstr::jal:
                    return imm_op::jal;
                case LerosInstr::stindb: case LerosInstr::ldindb:
                    return imm_op::loadi;
                case LerosInstr::stindh: case LerosInstr::ldindh:
                    return imm_op::shl1;
                case LerosInstr::stind: case LerosInstr::ldind:
                    return imm_op::shl2;
                default:
                    return imm_op::nop;
            }
            // clang-format on
        };

        alu_op1_ctrl << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::br:     case LerosInstr::brz:    case LerosInstr::brnz:   case LerosInstr::brp:
                case LerosInstr::brn:    case LerosInstr::jal:
                    return alu_op1_op::pc;
                case LerosInstr::ldind: case LerosInstr::ldindh: case LerosInstr::ldindb: case LerosInstr::stind:
                case LerosInstr::stindb: case LerosInstr::stindh:
                    return alu_op1_op::addr;
                default:
                    return alu_op1_op::acc;

            }
            // clang-format on
        };

        alu_op2_ctrl << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::addi: case LerosInstr::subi: case LerosInstr::andi: case LerosInstr::ori:
                case LerosInstr::xori: case LerosInstr::jal: case LerosInstr::br: case LerosInstr::brz:
                case LerosInstr::brnz: case LerosInstr::brp: case LerosInstr::brn: case LerosInstr::ldind:
                case LerosInstr::ldindh: case LerosInstr::ldindb: case LerosInstr::stind: case LerosInstr::stindb:
                case LerosInstr::stindh: case LerosInstr::loadi: case LerosInstr::loadhi: case LerosInstr::loadh2i:
                case LerosInstr::loadh3i:
                    return alu_op2_op::imm;
                case LerosInstr::add: case LerosInstr::sub: case LerosInstr::andr:
                case LerosInstr::orr: case LerosInstr::xorr:
                     return alu_op2_op::reg;
                default:
                    return alu_op2_op::unused;
            }
            // clang-format on
        };

        br_ctrl << [=] {
            // clang-format off
            Switch(instr_op, LerosInstr) {
                case LerosInstr::br:     return br_op::br;
                case LerosInstr::brz:    return br_op::brz;
                case LerosInstr::brnz:   return br_op::brnz;
                case LerosInstr::brp:    return br_op::brp;
                case LerosInstr::brn:    return br_op::brn;
                default:            return br_op::nop;

            }
            // clang-format on
        };

        addr_reg_src_ctrl << [=] {
            Switch(instr_op, LerosInstr) {
                case LerosInstr::ldaddr:
                    return addr_reg_src::reg;
                default:
                    return addr_reg_src::addrreg;
            }
        };

        reg_data_src_ctrl << [=] {
            Switch(instr_op, LerosInstr) {
                case LerosInstr::jal:
                    return reg_data_src::alu;
                case LerosInstr::store:
                    return reg_data_src::acc;
                default:
                    return reg_data_src::nop;
            }
        };
    }

    INPUTPORT_ENUM(instr_op, LerosInstr);

    OUTPUTPORT_ENUM(reg_data_src_ctrl, reg_data_src);
    OUTPUTPORT_ENUM(addr_reg_src_ctrl, addr_reg_src);
    OUTPUTPORT_ENUM(acc_reg_src_ctrl, acc_reg_src);
    OUTPUTPORT_ENUM(alu_ctrl, alu_op);
    OUTPUTPORT_ENUM(imm_ctrl, imm_op);
    OUTPUTPORT_ENUM(alu_op1_ctrl, alu_op1_op);
    OUTPUTPORT_ENUM(alu_op2_ctrl, alu_op2_op);
    OUTPUTPORT_ENUM(br_ctrl, br_op);
    OUTPUTPORT_ENUM(dm_access_size, access_size_op);
    OUTPUTPORT_ENUM(dm_op, mem_op);
    OUTPUTPORT_ENUM(reg_op, mem_op);
};

}  // namespace leros
}  // namespace vsrtl
