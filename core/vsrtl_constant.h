#ifndef CONSTANT_H
#define CONSTANT_H

#include "vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"

namespace vsrtl {

namespace {
constexpr bool valueFitsInBitWidth(unsigned int width, int value) {
    const int v = value < 0 ? -value : value;
    // -1 to verify that there is space for the sign bit
    return ceillog2(v) <= width;
}
}  // namespace

/**
 * @param width Must be able to contain the signed bitfield of value
 *
 */
DefineGraphicsType(Constant);
template <unsigned int W>
class Constant : public Component {
public:
    SetGraphicsType(Constant);
    Constant(std::string name, Component* parent, VSRTL_VT_U value = 0) : Component(name, parent) {
        m_value = value;
        if (!valueFitsInBitWidth(W, m_value)) {
            throw std::runtime_error("Value does not fit inside provided bit-width");
        }

        out << ([=] { return m_value; });
    }

    OUTPUTPORT(out, W);

private:
    unsigned int m_width;
    VSRTL_VT_U m_value;
};

template <unsigned int W>
void operator>>(VSRTL_VT_S c, Port<W>& toThis) {
    // A constant should be created as a child of the shared parent between the component to be connected and the newly
    // created constant
    auto* parent = toThis.getParent()->getParent();
    auto* constant = parent->template create_component<Constant<W>>(
        "constant #" + std::to_string(parent->reserveConstantId()) + "v:" + std::to_string(c), c);
    constant->out >> toThis;
}

template <unsigned int W>
void operator>>(VSRTL_VT_U c, std::vector<Port<W>*> toThis) {
    for (auto& p : toThis)
        c >> *p;
}

}  // namespace vsrtl

#endif  // CONSTANT_H
