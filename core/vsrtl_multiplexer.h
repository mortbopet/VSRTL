#ifndef VSRTL_MULTIPLEXER_H
#define VSRTL_MULTIPLEXER_H

#include <array>
#include "vsrtl_component.h"

namespace vsrtl {

/**
 * @brief The Multiplexer class
 * Control signal for the multiplexer is always ins[0].
 */

class Multiplexer : public Component {
public:
    std::type_index getTypeId() const override { return std::type_index(typeid(Multiplexer)); }
    Multiplexer(std::string name, unsigned int nInputs, unsigned int width, Component* parent)
        : Component(name, parent), m_nInputs(nInputs), m_width(width) {
        select.setWidth(ceillog2(m_nInputs));
        out.setWidth(width);
        in = createInputPorts("in", m_nInputs);
        for (const auto& i : in) {
            i->setWidth(width);
        }

        out << [=] { return in[select.template value<VSRTL_VT_U>()]->template value<VSRTL_VT_U>(); };
    }

    OUTPUTPORT(out);
    INPUTPORT(select);
    INPUTPORTS(in);

private:
    unsigned int m_width, m_nInputs;
};
}  // namespace vsrtl

#endif  // VSRTL_MULTIPLEXER_H
