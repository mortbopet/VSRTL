#ifndef ALU_H
#define ALU_H

#include <cstdint>
#include "ripes_primitive.h"

namespace ripes {

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
class ALU : public Primitive<width, /*Input ports:*/ 2, /*Additional inputs:*/ 1> {
public:
    ALU() : Primitive<width, 2, 1>("ALU") {}

    void propagate() override {
        this->propagateBase([=] { return calculateOutput(); });
    }

    void verifySubtype() const override {}

private:
    std::array<bool, width> calculateOutput() {
        uint32_t uop1 = static_cast<uint32_t>(*(this->m_inputs[0]));
        uint32_t uop2 = static_cast<uint32_t>(*(this->m_inputs[1]));
        int32_t op1 = static_cast<int32_t>(*(this->m_inputs[0]));
        int32_t op2 = static_cast<int32_t>(*(this->m_inputs[1]));
        switch ((ALU_OPCODE) static_cast<uint32_t>(*this->m_additionalInputs[0])) {
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
                return buildUnsignedArr<width>(op1 >> uop2);
                break;
            case ALU_OPCODE::SRL:
                return buildUnsignedArr<width>(uop1 >> uop2);
                break;
            case ALU_OPCODE::LUI:
                return buildUnsignedArr<width>(uop2);
                break;
            case ALU_OPCODE::LT:
                return buildUnsignedArr<width>(op1 < op2 ? 1 : 0);
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
}  // namespace ripes

#endif  // ALU_H
