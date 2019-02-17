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

// Signals cannot exist outside of components!

namespace vsrtl {

class Component;

// Interval value type of ports. Should be set according to the maximally representable number. If the
// static_asserts of Port<#> trigger, this indicates that VSRTL_BASE_VT should be set to a larger data type.
/** @note should be an unsigned data type */
using VSRTL_VT_U = unsigned int;
using VSRTL_VT_S = int;
static_assert(sizeof(VSRTL_VT_S) == sizeof(VSRTL_VT_U), "Base value types must be equal in size");

enum class PropagationState { unpropagated, propagated, constant };

class PortBase {
public:
    PortBase(std::string name, Component* parent) : m_parent(parent), m_name(name) {}
    bool isConnected() const { return m_portConnectsTo != nullptr || m_propagationFunction != nullptr; }
    virtual void propagate() = 0;
    virtual void propagateConstant() = 0;
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

    const std::vector<PortBase*>& getConnectsFromThis() const { return m_connectsFromThis; }
    PortBase* getConnectsToThis() const { return m_portConnectsTo; }

    // Port connections are doubly linked
    void operator>>(PortBase& toThis) {
        m_connectsFromThis.push_back(&toThis);
        if (toThis.m_portConnectsTo != nullptr) {
            throw std::runtime_error("Port input already connected");
        }
        if (m_width != toThis.getWidth()) {
            throw std::runtime_error("Port width mismatch");
        }
        toThis.m_portConnectsTo = this;
    }

    // Value access operators
    virtual explicit operator VSRTL_VT_S() const = 0;
    explicit operator VSRTL_VT_U() const { return m_value; }
    explicit operator bool() const { return m_value & 0b1; }

protected:
    // Port values are initialized to 0xdeadbeef for error detection reasons. In reality (in a circuit), this would not
    // be the case - the entire circuit is reset when the registers are reset (to 0), and the circuit state is then
    // propagated.
    VSRTL_VT_U m_value = 0xdeadbeef;

    // A port may only have a single input  ->[port]
    PortBase* m_portConnectsTo = nullptr;
    // A port may have multiple outputs     [port]->...->
    std::vector<PortBase*> m_connectsFromThis;

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

class Port : public PortBase {
public:
    Port(std::string name, Component* parent) : PortBase(name, parent) {}

    template <typename T>
    T value() {
        return static_cast<T>(signextend<T>(m_value, m_width));
    }
    explicit operator VSRTL_VT_S() const override { return signextend<int>(m_value, m_width); }

    void operator<<(std::function<VSRTL_VT_U()>&& propagationFunction) { m_propagationFunction = propagationFunction; }
    void propagate() override {
        if (m_propagationState == PropagationState::unpropagated) {
            setPortValue();
            // Propagate the value to the ports which connect to this
            for (auto& port : m_connectsFromThis)
                port->propagate();
            m_propagationState = PropagationState::propagated;
        }
    }

    void propagateConstant() override {
        m_propagationState = PropagationState::constant;
        setPortValue();
        for (auto& port : m_connectsFromThis)
            port->propagateConstant();
    }
};

}  // namespace vsrtl

#endif  // VSRTL_SIGNAL_H
