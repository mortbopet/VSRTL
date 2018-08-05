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
public:
    Constant() {
        m_output->setPropagationFunction([] {
            const static auto cArr = buildUnsignedArr<width>(constantValue);
            return cArr;
        });
    }

    OUTPUTSIGNAL(m_output, width);

    static_assert(valueFitsInBitWidth(width, constantValue), "Value does not fit inside provided bit-width");
};

// Connection operator
template <uint32_t width, int constantValue>
inline void operator>>(std::unique_ptr<Constant<width, constantValue>>& c, std::unique_ptr<Signal<width>*>& toInput) {
    *toInput = c->m_output.get();
}

}  // namespace ripes

#endif  // CONSTANT_H
