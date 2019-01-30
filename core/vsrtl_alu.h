#pragma once

#include <cstdint>
#include "vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_signal.h"

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
    // clang-format off
    ALU(const char* name) : Component(name){
        out.setPropagationFunction([=]{return calculateOutput();});
    }
    void propagate() { calculateOutput(); }

    INPUTSIGNAL(op1, width);
    INPUTSIGNAL(op2, width);
    INPUTSIGNAL(ctrl, ALUctrlWidth());

    OUTPUTSIGNAL(out, width);

private:
    std::array<bool, width> calculateOutput() {

        uint32_t uop1 = op1.template value<uint32_t>();
        uint32_t uop2 = op2.template value<uint32_t>();
        int32_t _op1 = op1.template value<int32_t>();
        int32_t _op2 = op2.template value<int32_t>();
        switch (ctrl.value<uint32_t>()) {
            case ALU_OPCODE::ADD:
                return buildUnsignedArr<width>(uop1 + uop2);
                break;
            case ALU_OPCODE::SUB:
                return buildUnsignedArr<width>(uop1 - uop2);
                break;
            case ALU_OPCODE::MUL:
                return buildUnsignedArr<width>(uop1 * uop2);
                break;
            case ALU_OPCODE::DIV:
                return buildUnsignedArr<width>(uop1 / uop2);
                break;
            case ALU_OPCODE::AND:
                return buildUnsignedArr<width>(uop1 & uop2);
                break;
            case ALU_OPCODE::OR:
                return buildUnsignedArr<width>(uop1 | uop2);
                break;
            case ALU_OPCODE::XOR:
                return buildUnsignedArr<width>(uop1 ^ uop2);
                break;
            case ALU_OPCODE::SL:
                return buildUnsignedArr<width>(uop1 << uop2);
                break;
            case ALU_OPCODE::SRA:
                return buildUnsignedArr<width>(_op1 >> uop2);
                break;
            case ALU_OPCODE::SRL:
                return buildUnsignedArr<width>(uop1 >> uop2);
                break;
            case ALU_OPCODE::LUI:
                return buildUnsignedArr<width>(uop2);
                break;
            case ALU_OPCODE::LT:
                return buildUnsignedArr<width>(_op1 < _op2 ? 1 : 0);
                break;
            case ALU_OPCODE::LTU:
                return buildUnsignedArr<width>(uop1 < uop2 ? 1 : 0);
                break;
            default:
                throw std::runtime_error("Invalid ALU opcode");
                break;
        }
    }
};
}  // namespace vsrtl
