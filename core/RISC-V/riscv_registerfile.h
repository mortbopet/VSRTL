#ifndef RISCV_REGISTERFILE_H
#define RISCV_REGISTERFILE_H

#include "../vsrtl_registerfile.h"

#include "../vsrtl_binutils.h"

namespace vsrtl {

class RISCV_RegisterFile : public RegisterFile<2> {
public:
    RISCV_RegisterFile() {
        // the instruction decoder should be specified as per the instruction set that you use. In case of RISC-V,
        // register numbers 1 and 2 are always in the same place, and thus we can use generateBitfieldDecoder to create
        // a simple function for extracting the bitfields. For more complex instruction sets, a custom lambda for
        // extracting the fields should be specified
        instructionDecoder = generateBitFieldDecoder(std::array<R_UINT, 6>{7, 5, 3, 5, 5, 7});  // from LSB to MSB);

        // Create functor for storing inputs of the register file - this is done before setting the output of
        // the register file
        auto rfWriter = [=] {
            if (writeEnable.value<bool>()) {
                m_reg[SIGNAL_VALUE(writeRegister, uint32_t)] = SIGNAL_VALUE(writeData, uint32_t);
            }
        };

        // For illustration, each step in setting the propagation function for each operand is described here:
        getOperand<0>()->setPropagationFunction([=] {
            rfWriter();
            // Get instruction
            const auto instr = SIGNAL_VALUE(instruction, uint32_t);
            // Decode instruction into its separate bit-fields
            const auto instructionFields = instructionDecoder(instr);
            // Get the register number from the instruction - in this case, rs1 is specified in field 3 (0-indexed)
            const auto registerNumber = instructionFields[3];
            // Read the register value
            const auto registerValue = m_reg[registerNumber];
            return buildUnsignedArr<REGISTERWIDTH>(registerValue);
        });

        getOperand<1>()->setPropagationFunction([=] {
            rfWriter();
            return buildUnsignedArr<REGISTERWIDTH>(m_reg[instructionDecoder(SIGNAL_VALUE(instruction, uint32_t))[4]]);
        });
    }

private:
    bitFieldDecoder instructionDecoder;
};
}  // namespace vsrtl

#endif  // RISCV_REGISTERFILE_H
