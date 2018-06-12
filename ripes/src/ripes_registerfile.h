#ifndef REGISTERFILE_H
#define REGISTERFILE_H

#include "ripes_assignable.h"
#include "ripes_defines.h"
#include "ripes_primitive.h"

namespace ripes {

class RegisterFile : public Primitive<REGISTERWIDTH, 1, 3> {
    // friend class Architecture;

public:
    RegisterFile() : Primitive("Register file") {
        // use std::get to do compile time check of m_additionalInputs array size
        std::get<0>(m_additionalInputs) = m_writeRegister;
        std::get<1>(m_additionalInputs) = m_writeData;
        std::get<2>(m_additionalInputs) = m_instruction;

        // Set assignable functors
    }

    void reset() {
        for (auto& reg : m_reg) {
            reg = 0;
        }
    }

    void propagate() override {}
    void verifySubtype() const override {}

    // Outputs - Public, so implementing Architecture can set destinations
    Assignable<REGISTERWIDTH> m_readData1;
    Assignable<REGISTERWIDTH> m_readData2;

private:
    // Inputs

    std::shared_ptr<Primitive<5>> m_writeRegister;
    std::shared_ptr<Primitive<REGISTERWIDTH>> m_writeData;
    std::shared_ptr<Primitive<REGISTERWIDTH>> m_instruction;

    // Registers
    std::vector<uint32_t> m_reg = std::vector<uint32_t>(REGISTERCOUNT, 0);
};
}  // namespace ripes

#endif  // REGISTERFILE_H
