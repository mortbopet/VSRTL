#pragma once

#include "vsrtl_component.h"

#include "common.h"

namespace vsrtl {
using namespace core;

namespace leros {

class Decode : public Component {
public:
    Decode(const std::string& name, SimComponent* parent) : Component(name, parent) {
        lowByte << [=] { return instr.uValue() & 0xFF; };

        op << [=] {
            const uint8_t instruction = instr.uValue() >> 8;

            // clang-format off
            switch ((instruction >> 4) & 0xF) {
                default: break;
                case 0b1000: return LerosInstr::br;
                case 0b1001: return LerosInstr::brz;
                case 0b1010: return LerosInstr::brnz;
                case 0b1011: return LerosInstr::brp;
                case 0b1100: return LerosInstr::brn;
            }

            switch(instruction){
                default: assert(false && "Could not match opcode");
                case 0x0: return LerosInstr::nop;
                case 0x08: return LerosInstr::add;
                case 0x09: return LerosInstr::addi;
                case 0x0c: return LerosInstr::sub;
                case 0x0d: return LerosInstr::subi;
                case 0x10: return LerosInstr::shra;
                case 0x20: return LerosInstr::load;
                case 0x21: return LerosInstr::loadi;
                case 0x22: return LerosInstr::andr;
                case 0x23: return LerosInstr::andi;
                case 0x24: return LerosInstr::orr;
                case 0x25: return LerosInstr::ori;
                case 0x26: return LerosInstr::xorr;
                case 0x27: return LerosInstr::xori;
                case 0x29: return LerosInstr::loadhi;
                case 0x2a: return LerosInstr::loadh2i;
                case 0x2b: return LerosInstr::loadh3i;
                case 0x30: return LerosInstr::store;
                case 0x39: return LerosInstr::ioout;
                case 0x05: return LerosInstr::ioin;
                case 0x40: return LerosInstr::jal;
                case 0x50: return LerosInstr::ldaddr;
                case 0x60: return LerosInstr::ldind;
                case 0x61: return LerosInstr::ldindb;
                case 0x62: return LerosInstr::ldindh;
                case 0x70: return LerosInstr::stind;
                case 0x71: return LerosInstr::stindb;
                case 0x72: return LerosInstr::stindh;
                case 0xff: return LerosInstr::scall;
            }
            // clang-format on
        };
    }

    INPUTPORT(instr, LEROS_INSTR_WIDTH);
    OUTPUTPORT_ENUM(op, LerosInstr);
    OUTPUTPORT(lowByte, LEROS_INSTR_WIDTH / 2);
};

}  // namespace leros
}  // namespace vsrtl
