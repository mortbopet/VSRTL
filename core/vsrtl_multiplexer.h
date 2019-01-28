#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include "vsrtl_primitive.h"

namespace vsrtl {

/**
 * @brief The Multiplexer class
 * Control signal for the multiplexer is always ins[0].
 */

template <uint32_t inputCount, uint32_t width>
class Multiplexer : public Component<width> {
public:
    Multiplexer() : Component("MUX") { this->m_additionalInputs.insert(0, m_control); }

    void propagate() override {
        propagateComponent([=] { return this->ins[m_control->getValue()]; });
    }

private:
    Component<ceillog2(inputCount)>* m_control;
};
}  // namespace vsrtl

#endif  // MULTIPLEXER_H
