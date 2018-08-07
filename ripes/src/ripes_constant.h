#ifndef CONSTANT_H
#define CONSTANT_H

#include "ripes_binutils.h"
#include "ripes_component.h"
#include "ripes_signal.h"

namespace ripes {

namespace {
constexpr bool valueFitsInBitWidth(uint32_t width, int value) {
    const int v = value < 0 ? -value : value;
    // -1 to verify that there is space for the sign bit
    return ceillog2(v) <= width;
}
}  // namespace

/**
 * @param width Must be able to contain the signed bitfield of value
 *
 */
template <uint32_t width, int constantValue>
class Constant : public Component {
    NON_REGISTER_COMPONENT
public:
    Constant() : Component("Constant") {
        out->setPropagationFunction([] {
            const static auto cArr = buildUnsignedArr<width>(constantValue);
            return cArr;
        });
    }

    OUTPUTSIGNAL(out, width);

    static_assert(valueFitsInBitWidth(width, constantValue), "Value does not fit inside provided bit-width");
};

// Connection operator
template <uint32_t width, int constantValue>
void operator>>(const Constant<width, constantValue>*& c, Signal<width>***& toInput) {
    c->out >> toInput;
}

}  // namespace ripes

#endif  // CONSTANT_H
