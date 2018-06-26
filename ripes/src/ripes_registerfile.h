#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include "ripes_assignable.h"
#include "ripes_defines.h"
#include "ripes_primitive.h"

namespace ripes {
/**
 * @brief The RegisterFile class
 * Inputs:
 *      1. Instruction : Instruction from which to decode register nOperands 1 and 2
 *
 * Additional inputs:
 *      1. writeRegister    : [1:31], register to write to
 *      2. writeEnable      : Triggers writing of input from writeData to writeRegister
 *      3. writeData        : Data to write to register
 *
 * Implementing architecture does not know about m_readData values in its Primitive container, so resetting,
 * propagating and verifying is done manually
 */
template <int nOperands>
class RegisterFile : public Primitive<REGISTERWIDTH, 1, 3> {
public:
    static_assert(nOperands > 0 && nOperands <= REGISTERCOUNT, "Register file invariant");

    enum AdditionalInputs { writeRegister = 0, writeEnable = 1, writeData = 2 };

    RegisterFile() : Primitive("Register file") {
        for (int i = 0; i < nOperands; i++) {
            m_operands[i] = std::make_shared<Assignable<REGISTERWIDTH>>();
        }
    }

    void resetPropagation() override {
        PrimitiveBase::resetPropagation();
        for (auto& operand : m_operands) {
            operand->resetPropagation();
        }
    }

    void reset() {
        for (auto& reg : m_reg) {
            reg = 0;
        }
    }

    void propagate() override {
        // Write data to registers
        if (this->m_additionalInputs[writeEnable]) {
            m_reg[this->m_additionalInputs[writeRegister]->getValue()] =
                this->m_additionalInputs[writeData]->getValue();
        }

        // Propagate operands (read data from registers)
        for (auto& operand : m_operands) {
            operand->propagate();
        }
    }
    void verifySubtype() const override {
        for (auto& operand : m_operands) {
            operand->verify();
        }
    }

    template <int operand>
    std::shared_ptr<Assignable<REGISTERWIDTH>> getOperand() {
        static_assert(operand >= 0 && operand < nOperands, "Operand not available");
        return m_operands[operand];
    }

protected:
    // Operand outputs - Public, so implementing Architecture can set destinations
    std::array<std::shared_ptr<Assignable<REGISTERWIDTH>>, nOperands> m_operands;

    // Registers
    std::vector<uint32_t> m_reg = std::vector<uint32_t>(REGISTERCOUNT, 0);
};
}  // namespace ripes

#endif  // REGISTERFILE_H
