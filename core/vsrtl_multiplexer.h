#ifndef VSRTL_MULTIPLEXER_H
#define VSRTL_MULTIPLEXER_H

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
    Multiplexer(std::string name) : Component(name) {
        out << [=] { return in[select.template value<VSRTL_VT_U>()]->template value<VSRTL_VT_U>(); };
    }

    OUTPUTPORT(out, width);
    INPUTPORT(select, ceillog2(inputCount));
    INPUTPORTS(in, width, inputCount);

private:
};
}  // namespace vsrtl

#endif  // VSRTL_MULTIPLEXER_H
