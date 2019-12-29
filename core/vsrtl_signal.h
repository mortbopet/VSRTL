#pragma once

namespace vsrtl {
namespace core {

/** @class Signal
 * the signal class represents component-local assignments of values which are to be passed as an output or further on
 * to subcomponents of a component. Similar to signals in VHDL, wires in Verilog.
 * A signal will be instantiated as a component local to the instantiating component.
 * Signals are only defined to have an output, which will be assigned a propagation function by the utilizing component.
 * To ensure that the signal is propagated correctly, >all< ports used in the signal's propagation function must be
 * added to its sensitivity list.
 */

#define SIGNAL(name, outWidth)                                                                  \
    class Signal_##name : public Component {                                                    \
    public:                                                                                     \
        SetGraphicsType(Signal);                                                                \
        Signal_##name(std::string name, SimComponent* parent) : Component(name, parent) {}      \
                                                                                                \
        OUTPUTPORT(out, outWidth);                                                              \
                                                                                                \
    protected:                                                                                  \
        void verifyComponent() const override {                                                 \
            if (m_sensitivityList.empty()) {                                                    \
                throw std::runtime_error("Signal: '" + getName() +                              \
                                         "' is not sensitive to anything, will never update'"); \
            }                                                                                   \
            Component::verifyComponent();                                                       \
        }                                                                                       \
    };                                                                                          \
    SUBCOMPONENT(name, Signal_##name);

}  // namespace core
}  // namespace vsrtl
