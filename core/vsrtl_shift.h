#ifndef VSRTL_SHIFT_H
#define VSRTL_SHIFT_H

#include "vsrtl_component.h"

namespace vsrtl {
namespace core {

enum class ShiftType { sl, sra, srl };

template <unsigned int W>
class Shift : public Component {
public:
    Shift(std::string name, SimComponent* parent, ShiftType t, unsigned int shamt) : Component(name, parent) {
        out << [=] {
            if (t == ShiftType::sl) {
                return in.template value<VSRTL_VT_U>() << shamt;
            } else if (t == ShiftType::sra) {
                return static_cast<VSRTL_VT_U>(in.template value<VSRTL_VT_S>() >> shamt);
            } else if (t == ShiftType::srl) {
                return in.template value<VSRTL_VT_U>() >> shamt;
            } else {
                throw std::runtime_error("Unknown shift type");
            }
        };
    }

    OUTPUTPORT(out, W);
    INPUTPORT(in, W);
};

}  // namespace core
}  // namespace vsrtl
#endif  // VSRTL_SHIFT_H
