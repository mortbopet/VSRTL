#ifndef VSRTL_MULTIPLEXER_H
#define VSRTL_MULTIPLEXER_H

#include <array>
#include "vsrtl_component.h"

namespace vsrtl {

/**
 * @brief The Multiplexer class
 * Control signal for the multiplexer is always ins[0].
 */

#define MUX_SELECT select

class Multiplexer : public Component {
public:
    const char* getBaseType() const override { return "Multiplexer"; }
    Multiplexer(std::string name, unsigned int nInputs, unsigned int width)
        : Component(name), m_nInputs(nInputs), m_width(width) {
        MUX_SELECT.setWidth(ceillog2(m_nInputs));
        out.setWidth(width);
        in = createInputPorts("in", m_nInputs);
        for (const auto& i : in) {
            i->setWidth(width);
        }

        out << [=] { return in[select.template value<VSRTL_VT_U>()]->template value<VSRTL_VT_U>(); };

        for (const auto& p : m_inputports) {
            p->setWidth(width);
        }
    }

    OUTPUTPORT(out);
    INPUTPORT(MUX_SELECT);
    INPUTPORTS(in);

private:
    unsigned int m_width, m_nInputs;
};
}  // namespace vsrtl

#endif  // VSRTL_MULTIPLEXER_H
