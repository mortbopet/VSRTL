#ifndef VSRTL_ADDERANDREG_H
#define VSRTL_ADDERANDREG_H

#include "vsrtl_adder.h"
#include "vsrtl_constant.h"
#include "vsrtl_design.h"
#include "vsrtl_register.h"

namespace vsrtl {
namespace core {

class AdderAndReg : public Design {
public:
    AdderAndReg() : Design("Adder and Register") {
        // Connect objects
        4 >> adder->op1;
        reg->out >> adder->op2;
        adder->out >> reg->in;
    }
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT(adder, Adder<32>);
    SUBCOMPONENT(reg, Register<32>);
};

}  // namespace core
}  // namespace vsrtl
#endif  // VSRTL_ADDERANDREG_H
