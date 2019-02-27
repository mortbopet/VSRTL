#ifndef VSRTL_SIGNAL_H
#define VSRTL_SIGNAL_H

#include <limits.h>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <type_traits>

#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"

#include "Signals/Signal.h"

namespace vsrtl {

class Component;

enum class PropagationState { unpropagated, propagated, constant };

class Port {
public:
    Port(std::string name, Component* parent, unsigned int width = 0)
        : m_parent(parent), m_name(name), m_width(width) {}
    bool isConnected() const { return m_portConnectsTo != nullptr || m_propagationFunction != nullptr; }
    std::string getName() const { return m_name; }
    Component* getParent() const { return m_parent; }
    bool isPropagated() const { return m_propagationState != PropagationState::unpropagated; }
    void resetPropagation() {
        m_propagationState =
            m_propagationState == PropagationState::constant ? m_propagationState : PropagationState::unpropagated;
    }

    unsigned int getWidth() const { return m_width; }
    void setWidth(unsigned int width) {
        assert(width <= sizeof(VSRTL_VT_U) * CHAR_BIT &&
               "Port value can not be represented by the current VSRTL base value type");
        m_width = width;
    }

    const std::vector<Port*>& getConnectsFromThis() const { return m_connectsFromThis; }
    Port* getConnectsToThis() const { return m_portConnectsTo; }

    // Port connections are doubly linked
    void operator>>(Port& toThis) {
        m_connectsFromThis.push_back(&toThis);
        if (toThis.m_portConnectsTo != nullptr) {
            throw std::runtime_error("Port input already connected");
        }
        if (m_width == 0) {
            throw std::runtime_error("Port width not initialized");
        }
        if (m_width != toThis.getWidth()) {
            throw std::runtime_error("Port width mismatch");
        }
        toThis.m_portConnectsTo = this;
    }

    template <typename T>
    T value() {
        return static_cast<T>(signextend<T>(m_value, m_width));
    }

    explicit operator VSRTL_VT_S() const { return signextend<VSRTL_VT_S>(m_value, m_width); }

    void operator<<(std::function<VSRTL_VT_U()>&& propagationFunction) { m_propagationFunction = propagationFunction; }
    void propagate() {
        auto prePropagateValue = m_value;
        if (m_propagationState == PropagationState::unpropagated) {
            setPortValue();
            // Propagate the value to the ports which connect to this
            for (auto& port : m_connectsFromThis)
                port->propagate();
            m_propagationState = PropagationState::propagated;

            // Signal all watcher of this port that the port value changed
            if (m_value != prePropagateValue) {
                changed.Emit();
            }
        }
    }

    void propagateConstant() {
        m_propagationState = PropagationState::constant;
        setPortValue();
        for (auto& port : m_connectsFromThis)
            port->propagateConstant();
    }

    Gallant::Signal0<> changed;

    // Value access operators
    explicit operator VSRTL_VT_U() const { return m_value; }
    explicit operator bool() const { return m_value & 0b1; }

protected:
    // Port values are initialized to 0xdeadbeef for error detection reasons. In reality (in a circuit), this would not
    // be the case - the entire circuit is reset when the registers are reset (to 0), and the circuit state is then
    // propagated.
    VSRTL_VT_U m_value = 0xdeadbeef;

    // A port may only have a single input  ->[port]
    Port* m_portConnectsTo = nullptr;
    // A port may have multiple outputs     [port]->...->
    std::vector<Port*> m_connectsFromThis;

    void setPortValue() {
        if (m_propagationFunction != nullptr) {
            m_value = m_propagationFunction();
        } else {
            m_value = static_cast<VSRTL_VT_U>(*m_portConnectsTo);
        }
    }

    std::function<VSRTL_VT_U()> m_propagationFunction;
    PropagationState m_propagationState = PropagationState::unpropagated;
    Component* m_parent;
    unsigned int m_width = 0;

private:
    std::string m_name;
};

}  // namespace vsrtl

#endif  // VSRTL_SIGNAL_H
