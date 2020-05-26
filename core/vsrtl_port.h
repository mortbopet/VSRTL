#ifndef VSRTL_SIGNAL_H
#define VSRTL_SIGNAL_H

#include <limits.h>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <type_traits>

#include "../interface/vsrtl_binutils.h"
#include "vsrtl_defines.h"

namespace vsrtl {
namespace core {

class Component;

enum class PropagationState { unpropagated, propagated, constant };

class Port : public SimPort {
public:
    Port(std::string name, unsigned int W, SimComponent* parent) : SimPort(name, parent), m_W(W) {
        assert(parent != nullptr);
    }
    bool isConnected() const { return m_inputPort != nullptr || m_propagationFunction; }

    bool isPropagated() const { return m_propagationState != PropagationState::unpropagated; }
    bool isConstant() const override { return m_propagationState == PropagationState::constant; }
    void resetPropagation() {
        m_propagationState =
            m_propagationState == PropagationState::constant ? m_propagationState : PropagationState::unpropagated;
    }

    // Port connections are doubly linked
    void operator>>(Port& toThis) {
        if (toThis.getWidth() != getWidth()) {
            throw std::runtime_error("Failed trying to connect port '" + getParent()->getName() + ":" + getName() +
                                     "' to port '" + toThis.getParent()->getName() + ":" + toThis.getName() +
                                     ". Port width mismatch '");
        }
        if (toThis.m_inputPort != nullptr) {
            throw std::runtime_error(
                "Failed trying to connect port '" + getParent()->getName() + ":" + getName() + "' to port '" +
                toThis.getParent()->getName() + ":" + toThis.getName() + ". Port is already connected to '" +
                toThis.getInputPort()->getParent()->getName() + ":" + toThis.getInputPort()->getName());
        }
        toThis.m_inputPort = this;
        m_outputPorts.push_back(&toThis);
    }

    void operator>>(const std::vector<Port*>& toThis) {
        for (auto& p : toThis)
            *this >> *p;
    }

    template <typename T>
    T value() const {
        return static_cast<T>(signextend<T>(m_value, m_W));
    }

    VSRTL_VT_U uValue() const override { return value<VSRTL_VT_U>(); }
    VSRTL_VT_S sValue() const override { return value<VSRTL_VT_S>(); }
    unsigned int getWidth() const override { return m_W; }

    explicit operator VSRTL_VT_S() const { return signextend<VSRTL_VT_S>(m_value, m_W); }

    void setPortValue() {
        auto prePropagateValue = m_value;
        if (m_propagationFunction) {
            m_value = m_propagationFunction();
        } else {
            m_value = getInputPort<Port>()->template value<VSRTL_VT_U>();
        }
        if (m_value != prePropagateValue) {
            // Signal all watcher of this port that the port value changed
            if (getDesign()->signalsEnabled()) {
                changed.Emit();
            }
        }
    }

    void propagate(std::vector<Port*>& propagationStack) {
        if (m_propagationState == PropagationState::unpropagated) {
            propagationStack.push_back(this);
            // Propagate the value to the ports which connect to this
            for (const auto& port : getOutputPorts<Port>())
                port->propagate(propagationStack);
            m_propagationState = PropagationState::propagated;
        }
    }

    void propagateConstant() {
        m_propagationState = PropagationState::constant;
        setPortValue();
        for (const auto& port : getOutputPorts<Port>())
            port->propagateConstant();
    }

    void operator<<(std::function<VSRTL_VT_U()>&& propagationFunction) {
        if (m_propagationFunction) {
            throw std::runtime_error("Propagation function reassignment prohibited");
        }
        m_propagationFunction = propagationFunction;
    }

    // Value access operators
    explicit operator VSRTL_VT_U() const { return m_value; }
    explicit operator bool() const { return m_value & 0b1; }

    /**
     * @brief stringValue
     * A port may define special string formatting to be displayed in the graphical library. If so, owning components
     * should set the string value function to provide such values.
     */
    virtual bool isEnumPort() const override { return false; }
    virtual std::string valueToEnumString() const override { throw std::runtime_error("This is not an enum port!"); }
    virtual VSRTL_VT_U enumStringToValue(const char*) const override {
        throw std::runtime_error("This is not an enum port!");
    }

protected:
    // Port values are initialized to 0xdeadbeef for error detection reasons. In reality (in a circuit), this would
    // not be the case - the entire circuit is reset when the registers are reset (to 0), and the circuit state is
    // then propagated.
    VSRTL_VT_U m_value = 0xdeadbeef;

    std::function<VSRTL_VT_U()> m_propagationFunction = {};
    PropagationState m_propagationState = PropagationState::unpropagated;
    const unsigned int m_W;
};

template <typename E_t>
class EnumPort : public Port {
public:
    EnumPort(std::string name, unsigned int W, SimComponent* parent) : Port(name, W, parent) {}

    bool isEnumPort() const override { return true; }
    std::string valueToEnumString() const override {
        return E_t::_from_integral(this->template value<VSRTL_VT_U>())._to_string();
    }
    VSRTL_VT_U enumStringToValue(const char* str) const override { return E_t::_from_string(str); }
};

}  // namespace core
}  // namespace vsrtl

#endif  // VSRTL_SIGNAL_H
