#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include "ripes_component.h"
#include "ripes_defines.h"
#include "ripes_signal.h"

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
class RegisterFile : public Component {
public:
    static_assert(nOperands > 0 && nOperands <= REGISTERCOUNT, "Register file invariant");

    RegisterFile() : Component("Register File") {}

    INPUTSIGNAL(m_instruction, REGISTERWIDTH);
    INPUTSIGNAL(m_writeRegister, REGISTERWIDTH);
    INPUTSIGNAL(m_writeEnable, REGISTERWIDTH);
    INPUTSIGNAL(m_writeData, REGISTERWIDTH);

    void reset() {
        for (auto& reg : m_reg) {
            reg = 0;
        }
    }

    template <int operand>
    Signal<REGISTERWIDTH>* getOperand() {
        static_assert(operand >= 0 && operand < nOperands, "Operand not available");
        return m_operands[operand].get();
    }
    std::array<std::unique_ptr<Signal<REGISTERWIDTH>>, nOperands> m_operands;

protected:
    // Registers
    std::vector<uint32_t> m_reg = std::vector<uint32_t>(REGISTERCOUNT, 0);
};
}  // namespace ripes

#endif  // REGISTERFILE_H
