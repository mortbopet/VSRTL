#pragma once

namespace vsrtl {
namespace core {

/** @class Wire
 * the wire class represents component-local assignments of values which are to
 * be passed as an output or further on to subcomponents of a component. Similar
 * to wires in VHDL, wires in Verilog. A wire will be instantiated as a
 * component local to the instantiating component. wires are only defined to
 * have an output, which will be assigned a propagation function by the
 * utilizing component. To ensure that the wire is propagated correctly, >all<
 * ports used in the wire's propagation function must be added to its
 * sensitivity list.
 */

#define WIRE(name, outWidth)                                                   \
  class wire_##name : public Component {                                       \
  public:                                                                      \
    SetGraphicsType(Wire);                                                     \
    wire_##name(const std::string &name, SimComponent *parent)                 \
        : Component(name, parent) {}                                           \
                                                                               \
    OUTPUTPORT(out, outWidth);                                                 \
                                                                               \
  protected:                                                                   \
    void verifyComponent() const override {                                    \
      if (m_sensitivityList.empty()) {                                         \
        throw std::runtime_error(                                              \
            "Wire: '" + getName() +                                            \
            "' is not sensitive to anything, will never update'");             \
      }                                                                        \
      Component::verifyComponent();                                            \
    }                                                                          \
  };                                                                           \
  SUBCOMPONENT(name, wire_##name);

} // namespace core
} // namespace vsrtl
