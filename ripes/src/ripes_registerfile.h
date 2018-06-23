#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include "ripes_assignable.h"
#include "ripes_defines.h"
#include "ripes_primitive.h"

namespace ripes {
/**
 * @brief The RegisterFile class
 * Inputs:
 *      1. Instruction : Instruction from which to decode register operands 1 and 2
 *
 * Additional inputs:
 *      1. writeRegister    : [1:31], register to write to
 *      2. writeEnable      : Triggers writing of input from writeData to writeRegister
 *      3. writeData        : Data to write to register
 *
 * Implementing architecture does not know about m_readData values in its Primitive container, so resetting,
 * propagating and verifying is done manually
 */
class RegisterFile : public Primitive<REGISTERWIDTH, 1, 3> {
public:
    enum AdditionalInputs { writeRegister = 0, writeEnable = 1, writeData = 2 };

    RegisterFile() : Primitive("Register file") {}

    void resetPropagation() override {
        PrimitiveBase::resetPropagation();
        m_readData1.resetPropagation();
        m_readData2.resetPropagation();
    }

    void reset() {
        for (auto& reg : m_reg) {
            reg = 0;
        }
    }

    void propagate() override {
        m_readData1.propagate();
        m_readData2.propagate();
    }
    void verifySubtype() const override {
        m_readData1.verify();
        m_readData2.verify();
    }

protected:
    // Outputs - Public, so implementing Architecture can set destinations
    Assignable<REGISTERWIDTH> m_readData1;
    Assignable<REGISTERWIDTH> m_readData2;

    // Registers
    std::vector<uint32_t> m_reg = std::vector<uint32_t>(REGISTERCOUNT, 0);
};
}  // namespace ripes

#endif  // REGISTERFILE_H
