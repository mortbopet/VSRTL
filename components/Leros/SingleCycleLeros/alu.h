#pragma once

#include "vsrtl_component.h"

#include "common.h"

namespace vsrtl {
using namespace core;

namespace leros {

class ALU : public Component {
public:
    ALU(const std::string& name, SimComponent* parent) : Component(name, parent) {
        res << [=] {
            const auto op1s = op1.sValue();
            const auto op2s = op2.sValue();
            Switch(ctrl, alu_op) {
                case alu_op::nop:
                    return VSRTL_VT_S(0);
                case alu_op::add:
                    return op1s + op2s;
                case alu_op::sub:
                    return op1s - op2s;
                case alu_op::shra:
                    return op1s >> 1;
                case alu_op::alu_and:
                    return op1s & op2s;
                case alu_op::alu_or:
                    return op1s | op2s;
                case alu_op::alu_xor:
                    return op1s ^ op2s;
                case alu_op::loadi:
                    return op2s;
                case alu_op::loadhi:
                    return VT_S((op2s & 0xFFFFFF00) | (op1s & 0xFF));
                case alu_op::loadh2i:
                    return VT_S((op2s & 0xFFFF0000) | (op1s & 0xFFFF));
                case alu_op::loadh3i:
                    return VT_S((op2s & 0xFF000000) | (op1s & 0xFFFFFF));
                default:
                    throw std::runtime_error("Unknown ALU op");
            }
        };
    }

    INPUTPORT(op1, LEROS_REG_WIDTH);
    INPUTPORT(op2, LEROS_REG_WIDTH);
    INPUTPORT_ENUM(ctrl, alu_op);
    OUTPUTPORT(res, LEROS_REG_WIDTH);
};

}  // namespace leros
}  // namespace vsrtl
