#ifndef CONSTANT_H
#define CONSTANT_H

#include "vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_signal.h"

namespace vsrtl {

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
template <unsigned int width, int constantValue>
class Constant : public Component {
    NON_REGISTER_COMPONENT
public:
    Constant(const char* name) : Component(name) {
        value.setPropagationFunction([] {
            const static auto cArr = buildUnsignedArr<width>(constantValue);
            return cArr;
        });
    }

    OUTPUTSIGNAL(value, width);

    static_assert(valueFitsInBitWidth(width, constantValue), "Value does not fit inside provided bit-width");
};

}  // namespace vsrtl

#endif  // CONSTANT_H
