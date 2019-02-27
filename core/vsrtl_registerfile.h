#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_port.h"

#include <memory>

namespace vsrtl {
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

    RegisterFile(std::string name) : Component(name) {
        for (int i = 0; i < nOperands; i++) {
            auto& op = createOutputPort<REGISTERWIDTH>("Operand");
            operands.push_back(&op);
        }
    }

    INPUTPORT(instruction);
    INPUTPORT(writeRegister);
    INPUTPORT(writeEnable);
    INPUTPORT(writeData);

    void reset() {
        for (const auto& reg : m_reg) {
            reg = 0;
        }
    }

    uint32_t value(uint32_t index) const {
        if (index < 32) {
            return m_reg[index];
        } else {
            return 0;
        }
    }

    template <int operand>
    Port<REGISTERWIDTH>& getOperand() {
        static_assert(operand >= 0 && operand < nOperands, "Operand not available");
        return *operands[operand];
    }
    std::vector<Port<REGISTERWIDTH>*> operands;

protected:
    // Registers
    std::vector<uint32_t> m_reg = std::vector<uint32_t>(REGISTERCOUNT, 0);
};
}  // namespace vsrtl

#endif  // REGISTERFILE_H
