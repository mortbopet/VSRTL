#ifndef VSRTL_SHIFT_H
#define VSRTL_SHIFT_H

#include "vsrtl_component.h"

namespace vsrtl {

enum class ShiftType { sl, sra, srl };

template <unsigned int width, unsigned int n, ShiftType t>
class Shift : public Component {
    NON_REGISTER_COMPONENT
public:
    Shift(std::string name = "") : Component(name) {
        out << [=] {
            if constexpr (t == ShiftType::sl) {
                return in.template value<VSRTL_VT_U>() << n;
            } else if constexpr (t == ShiftType::sra) {
                return in.template value<VSRTL_VT_S>() >> n;
            } else if constexpr (t == ShiftType::srl) {
                return in.template value<VSRTL_VT_U>() >> n;
            }
        };
    }

    OUTPUTPORT(out, width);
    INPUTPORT(in, width);
};

}  // namespace vsrtl
#endif  // VSRTL_SHIFT_H
