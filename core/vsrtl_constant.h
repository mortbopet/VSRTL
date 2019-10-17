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
DefineGraphicsProxy(Constant);
template <unsigned int W>
class Constant : public Component {
public:
    DefineTypeID(Constant);
    Constant(std::string name, Component* parent, VSRTL_VT_U value = 0) : Component(name, parent) {
        m_value = value;
        if (!valueFitsInBitWidth(W, m_value)) {
            throw std::runtime_error("Value does not fit inside provided bit-width");
        }

        out << ([=] { return m_value; });
    }

    OUTPUTPORT(out, W);

    // HACK HACK HACK
    void changeConstant(VSRTL_VT_U value) { m_value = value; }
    // HACK HACK HACK

private:
    unsigned int m_width;
    VSRTL_VT_U m_value;
};

}  // namespace vsrtl

#endif  // CONSTANT_H
