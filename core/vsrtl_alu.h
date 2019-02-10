#pragma once

#include <cstdint>
#include "vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_port.h"

namespace vsrtl {

enum ALU_OPCODE {
    ADD = 0,
    SUB = 1,
    MUL = 2,
    DIV = 3,
    AND = 4,
    OR = 5,
    XOR = 6,
    SL = 7,
    SRA = 8,
    SRL = 9,
    LUI = 10,
    LT = 11,
    LTU = 12,
    EQ = 13,
    COUNT  // Used for calculating bitwidth for OPCODE signal
};

static constexpr unsigned int ALUctrlWidth() {
    return ceillog2(ALU_OPCODE::COUNT);
}

template <uint32_t width>
class ALU : public Component {
    NON_REGISTER_COMPONENT

public:
    const char* getBaseType() const override { return "ALU"; }
    // clang-format off
    ALU(const char* name) : Component(name){
        out << ([=] { return calculateOutput(); });
    }
    void propagate() { calculateOutput(); }

    INPUTPORT(op1, width);
    INPUTPORT(op2, width);
    INPUTPORT(ctrl, ALUctrlWidth());

    OUTPUTPORT(out, width);

private:
    VSRTL_VT_U calculateOutput() {
        const auto uop1 = op1.template value<VSRTL_VT_U>();
        const auto uop2 = op2.template value<VSRTL_VT_U>();
        const auto _op1 = op1.template value<VSRTL_VT_S>();
        const auto _op2 = op2.template value<VSRTL_VT_S>();
        switch (ctrl.value<VSRTL_VT_U>()) {
            case ALU_OPCODE::ADD:
                return uop1 + uop2;
            case ALU_OPCODE::SUB:
                return uop1 - uop2;
            case ALU_OPCODE::MUL:
                return uop1 * uop2;
            case ALU_OPCODE::DIV:
                return uop1 / uop2;
            case ALU_OPCODE::AND:
                return uop1 & uop2;
            case ALU_OPCODE::OR:
                return uop1 | uop2;
            case ALU_OPCODE::XOR:
                return uop1 ^ uop2;
            case ALU_OPCODE::SL:
                return uop1 << uop2;
            case ALU_OPCODE::SRA:
                return _op1 >> uop2;
            case ALU_OPCODE::SRL:
                return uop1 >> uop2;
            case ALU_OPCODE::LUI:
                return uop2;
            case ALU_OPCODE::LT:
                return _op1 < _op2 ? 1 : 0;
            case ALU_OPCODE::LTU:
                return uop1 < uop2 ? 1 : 0;
            default:
                throw std::runtime_error("Invalid ALU opcode");

        }
    }
};
}  // namespace vsrtl
