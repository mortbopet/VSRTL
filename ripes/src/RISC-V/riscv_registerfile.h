#ifndef RISCV_REGISTERFILE_H
#define RISCV_REGISTERFILE_H

#include "../ripes_registerfile.h"

#include "../ripes_binutils.h"

namespace ripes {

class RISCV_RegisterFile : public RegisterFile {
public:
    RISCV_RegisterFile() {
        // the instruction decoder should be specified as per the instruction set that you use. In case of RISC-V,
        // register numbers 1 and 2 are always in the same place, and thus we can use generateBitfieldDecoder to create
        // a simple function for extracting the bitfields. For more complex instruction sets, a custom lambda for
        // extracting the fields should be specified
        instructionDecoder = generateBitFieldDecoder(std::array<R_UINT, 6>{7, 5, 3, 5, 5, 7});  // from LSB to MSB);

        // For illustration, each step is described:
        m_readData1.setFunctor([this] {
            // Get instruction
            const auto instruction = m_inputs[0]->getValue();
            // Decode instruction into its separate bit-fields
            const auto instructionFields = instructionDecoder(instruction);
            // Get the register number from the instruction - in this case, rs1 is specified in field 3 (0-indexed)
            const auto registerNumber = instructionFields[3];
            // Read the register value
            const auto registerValue = m_reg[registerNumber];
            return buildUnsignedArr<REGISTERWIDTH>(registerValue);
        });
        m_readData2.setFunctor(
            [this] { return buildUnsignedArr<REGISTERWIDTH>(m_reg[instructionDecoder(m_inputs[0]->getValue())[4]]); });
    }

private:
    bitFieldDecoder instructionDecoder;
};
}  // namespace ripes

#endif  // RISCV_REGISTERFILE_H
