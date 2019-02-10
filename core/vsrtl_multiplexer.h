#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <array>
#include "vsrtl_component.h"

namespace vsrtl {

/**
 * @brief The Multiplexer class
 * Control signal for the multiplexer is always ins[0].
 */

template <unsigned int inputCount, unsigned int width>
class Multiplexer : public Component {
public:
    const char* getBaseType() const override { return "Multiplexer"; }
    Multiplexer(const char* name) : Component(name) {}

    OUTPUTPORT(out, width);
    INPUTPORT(select, ceillog2(inputCount));

    std::array<Port<width>, inputCount> inputs;

private:
};
}  // namespace vsrtl

#endif  // MULTIPLEXER_H
