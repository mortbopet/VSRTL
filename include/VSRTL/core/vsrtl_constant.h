#ifndef CONSTANT_H
#define CONSTANT_H

#include "../interface/vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include "../interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {
namespace core {

namespace {
constexpr bool valueFitsInBitWidth(unsigned int width, int value) {
  const int v = value < 0 ? -value : value;
  unsigned v_width =
      ceillog2(v) +
      ((bitcount(value) == 1) && (value != 0) && (value != 1) ? 1 : 0);
  return v_width <= width;
}
} // namespace

/**
 * @param width Must be able to contain the signed bitfield of value
 *
 */
template <unsigned int W>
class Constant : public Component {
public:
  SetGraphicsType(Constant);
  Constant(const std::string &name, SimComponent *parent, VSRTL_VT_U value = 0)
      : Component(name, parent) {
    m_value = value;
    if (!valueFitsInBitWidth(W, m_value)) {
      throw std::runtime_error("Value does not fit inside provided bit-width");
    }

    out << ([=] { return m_value; });
  }

  OUTPUTPORT(out, W);

private:
  VSRTL_VT_U m_value;
};

template <unsigned int W>
void operator>>(VSRTL_VT_S c, Port<W> &toThis) {
  // A constant should be created as a child of the shared parent between the
  // component to be connected and the newly created constant
  auto *parent = toThis.getParent()->template getParent<SimComponent>();
  auto *constant = parent->template create_component<Constant<W>>(
      "constant id" + std::to_string(parent->reserveConstantId()) + "v" +
          std::to_string(c),
      c);
  constant->out >> toThis;
}

template <unsigned int W>
void operator>>(VSRTL_VT_U c, std::vector<Port<W> *> toThis) {
  for (auto &p : toThis)
    c >> *p;
}

} // namespace core
} // namespace vsrtl

#endif // CONSTANT_H
