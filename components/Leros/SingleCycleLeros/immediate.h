#pragma once

#include "vsrtl_component.h"

#include "common.h"

namespace vsrtl {
using namespace core;

namespace leros {

class Immediate : public Component {
public:
    Immediate(const std::string& name, SimComponent* parent) : Component(name, parent) {
        imm << [=] {
            const auto imm8 = instr.uValue() & 0xFF;
            const auto simm8 = signextend<8>(imm8);
            const auto imm12 = instr.uValue() & 0xFFF;
            Switch(ctrl, imm_op) {
                case imm_op::nop:
                    return VSRTL_VT_S(0);
                case imm_op::shl1:
                    return simm8 << 1;
                case imm_op::shl2:
                    return simm8 << 2;
                case imm_op::branch:
                    return signextend<12>(imm12) << 1;
                case imm_op::loadi:
                    return simm8;
                case imm_op::loadhi:
                    return simm8 << 8;
                case imm_op::loadh2i:
                    return simm8 << 16;
                case imm_op::loadh3i:
                    return simm8 << 24;
                case imm_op::jal:
                    return VSRTL_VT_S(2);
                default:
                    assert(false);
                    return VSRTL_VT_S();
            }
        };
    }

    INPUTPORT(instr, LEROS_INSTR_WIDTH);
    INPUTPORT_ENUM(ctrl, imm_op);
    OUTPUTPORT(imm, LEROS_REG_WIDTH);
};

}  // namespace leros
}  // namespace vsrtl
