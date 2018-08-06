#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include "ripes_component.h"
#include "ripes_defines.h"
#include "ripes_signal.h"

#include <memory>

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
    REGISTER_COMPONENT
public:
    static_assert(nOperands > 0 && nOperands <= REGISTERCOUNT, "Register file invariant");

    RegisterFile() : Component("Register File") {
        for (int i = 0; i < nOperands; i++) {
            operands.push_back(createOutputSignal<REGISTERWIDTH>());
        }
    }

    INPUTSIGNAL(instruction, REGISTERWIDTH);
    INPUTSIGNAL(writeRegister, REGISTERWIDTH);
    INPUTSIGNAL(writeEnable, REGISTERWIDTH);
    INPUTSIGNAL(writeData, REGISTERWIDTH);

    void reset() {
        for (auto& reg : m_reg) {
            reg = 0;
        }
    }

    uint32_t value(uint32_t index) const {
        if (index >= 0 || index < 32) {
            return m_reg[index];
        } else {
            return 0;
        }
    }

    template <int operand>
    Signal<REGISTERWIDTH>* getOperand() {
        static_assert(operand >= 0 && operand < nOperands, "Operand not available");
        return operands[operand].get();
    }
    std::vector<std::unique_ptr<Signal<REGISTERWIDTH>>> operands;

protected:
    // Registers
    std::vector<uint32_t> m_reg = std::vector<uint32_t>(REGISTERCOUNT, 0);
};
}  // namespace ripes

#endif  // REGISTERFILE_H
