#ifndef CONSTANT_H
#define CONSTANT_H

#include "ripes_binutils.h"
#include "ripes_primitive.h"

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
template <uint32_t width, int value>
class Constant : public Primitive<width> {
public:
    Constant() : Primitive("Constant") { buildArr<width>(m_value, value); }

    static_assert(valueFitsInBitWidth(width, value), "Value cannot fit inside specified width of signal");

    void propagate() override {
        this->propagateBase([=] { return this->m_value; });
    }

    void verifySubtype() const override {}
};
}  // namespace ripes

#endif  // CONSTANT_H
