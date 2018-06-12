#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include "ripes_primitive.h"

namespace ripes {

/**
 * @brief The Multiplexer class
 * Control signal for the multiplexer is always m_inputs[0].
 */

template <uint32_t inputCount, uint32_t width>
class Multiplexer : public Primitive<width> {
public:
    Multiplexer() : Primitive("MUX") { this->m_additionalInputs.insert(0, m_control); }

    void propagate() override {
        propagateBase([=] { return this->m_inputs[m_control->getValue()]; });
    }

    void verifySubtype() const override {
        ASSERT_CONNECTION_DEFINED(m_control);
        ASSERT_CONNECTION_EXPR(this->m_inputs.size() == width);
    }

private:
    Primitive<ceillog2(inputCount)>* m_control;
};
}  // namespace ripes

#endif  // MULTIPLEXER_H
