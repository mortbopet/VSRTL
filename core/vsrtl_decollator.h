#ifndef VSRTL_DECOLLATOR_H
#define VSRTL_DECOLLATOR_H

#include "vsrtl_component.h"
#include "vsrtl_port.h"

namespace vsrtl {
namespace core {

/**
 * @brief The Decollator class
 *           _____
 *           |   | s[0]
 * [3:0] s-> |   | s[1]
 *           |   | s[2]
 *           |___| d[3]
 */
template <unsigned int W>
class Decollator : public Component {
public:
  Decollator(const std::string &name, SimComponent *parent)
      : Component(name, parent) {
    for (int i = 0; i < W; i++) {
      *out[i] << [=] { return (VT_U(in) >> i) & 0b1; };
    }
  }

  OUTPUTPORTS(out, 1, W);
  INPUTPORT(in, W);
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_DECOLLATOR_H
