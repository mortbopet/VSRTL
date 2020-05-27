#ifndef CONSTANT_H
#define CONSTANT_H

#include "../interface/vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include "../interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {
namespace core {

/**
 * @param width Must be able to contain the signed bitfield of value
 *
 */
class Constant : public Component {
public:
    SetGraphicsType(Constant);
    Constant(std::string name, SimComponent* parent, VSRTL_VT_U value = 0, unsigned int W = 0)
        : Component(name, parent), m_W(W) {
        if (m_W == 0) {
            m_W = bitsToRepresentUValue(value);
        }
        m_value = value;
        if (!valueFitsInBitWidth(m_W, m_value)) {
            throw std::runtime_error("Value does not fit inside provided bit-width");
        }
        // Width has now been fixed, initialize port
        DYNP_OUT_INIT(out, m_W);

        *out << ([=] { return m_value; });
    }

    DYNP_OUT(out);

private:
    VSRTL_VT_U m_value;
    unsigned m_W;
};

void operator>>(VSRTL_VT_S c, Port& toThis) {
    // A constant should be created as a child of the shared parent between the component to be connected and the newly
    // created constant
    auto* parent = toThis.getParent()->template getParent<SimComponent>();
    if (toThis.getWidth() == 0) {
        throw std::runtime_error("Cannot implicitely construct Constant for an uninitialized port!");
    }
    auto* constant = parent->template create_component<Constant>(
        "constant #" + std::to_string(parent->reserveConstantId()) + "v:" + std::to_string(c), c, toThis.getWidth());
    *constant->out >> toThis;
}

void operator>>(VSRTL_VT_U c, std::vector<Port*> toThis) {
    for (auto& p : toThis)
        c >> *p;
}

}  // namespace core
}  // namespace vsrtl

#endif  // CONSTANT_H
