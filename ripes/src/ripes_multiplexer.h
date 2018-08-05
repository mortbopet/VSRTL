#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include "ripes_primitive.h"

namespace ripes {

/**
 * @brief The Multiplexer class
 * Control signal for the multiplexer is always m_inputs[0].
 */

template <uint32_t inputCount, uint32_t width>
class Multiplexer : public Component<width> {
public:
    Multiplexer() : Component("MUX") { this->m_additionalInputs.insert(0, m_control); }

    void propagate() override {
        propagateComponent([=] { return this->m_inputs[m_control->getValue()]; });
    }

private:
    Component<ceillog2(inputCount)>* m_control;
};
}  // namespace ripes

#endif  // MULTIPLEXER_H
