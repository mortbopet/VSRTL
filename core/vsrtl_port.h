#ifndef VSRTL_SIGNAL_H
#define VSRTL_SIGNAL_H

#include <limits.h>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <type_traits>

#include "vsrtl_base.h"
#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"

#include "Signals/Signal.h"

namespace vsrtl {

class Component;

enum class PropagationState { unpropagated, propagated, constant };

class Port : public Base {
public:
    Port(std::string name, Component* parent, unsigned int width = 0)
        : m_parent(parent), m_name(name), m_width(width) {}
    bool isConnected() const { return m_inputPort != nullptr || m_propagationFunction != nullptr; }
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

    const std::vector<Port*>& getOutputPorts() const { return m_outputPorts; }
    Port* getInputPort() const { return m_inputPort; }
    std::vector<Port*> getPortsInConnection() {
        std::vector<Port*> portsInConnection;
        traverseConnection([=](Port* port, std::vector<Port*>& ports) { ports.push_back(port); }, portsInConnection);
        return portsInConnection;
    }

    /* traverse from any given port towards its root (source) port, while executing nodeFunc(args...) in each port which
    is visited along the way*/
    template <typename F, typename... Args>
    void traverseToRoot(const F& nodeFunc, Args&... args);

    /* From this port, visit all directly and implicitely connected port to this port, and execute nodeFunc(args...) in
    each visited port */
    template <typename F, typename... Args>
    void traverseConnection(const F& nodeFunc, Args&... args);

    /* Traverse from any given port towards its endpoint sinks, executing nodeFunc(args...) in each visited port */
    template <typename F, typename... Args>
    void traverseToSinks(const F& nodeFunc, Args&... args);

    // Port connections are doubly linked
    void operator>>(Port& toThis) {
        m_outputPorts.push_back(&toThis);
        if (toThis.m_inputPort != nullptr) {
            throw std::runtime_error("Port input already connected");
        }
        if (m_width == 0) {
            throw std::runtime_error("Port width not initialized");
        }
        if (m_width != toThis.getWidth()) {
            throw std::runtime_error("Port width mismatch");
        }
        toThis.m_inputPort = this;
    }

    template <typename T>
    T value() {
        return static_cast<T>(signextend<T>(m_value, m_width));
    }

    explicit operator VSRTL_VT_S() const { return signextend<VSRTL_VT_S>(m_value, m_width); }

    void setPortValue() {
        auto prePropagateValue = m_value;
        if (m_propagationFunction != nullptr) {
            m_value = m_propagationFunction();
        } else {
            m_value = static_cast<VSRTL_VT_U>(*m_inputPort);
        }
        if (m_value != prePropagateValue) {
            // Signal all watcher of this port that the port value changed
            changed.Emit();
        }
    }

    void operator<<(std::function<VSRTL_VT_U()>&& propagationFunction) { m_propagationFunction = propagationFunction; }
    void propagate(std::vector<Port*>& propagationStack) {
        if (m_propagationState == PropagationState::unpropagated) {
            propagationStack.push_back(this);
            // Propagate the value to the ports which connect to this
            for (const auto& port : m_outputPorts)
                port->propagate(propagationStack);
            m_propagationState = PropagationState::propagated;
        }
    }

    void propagateConstant() {
        m_propagationState = PropagationState::constant;
        setPortValue();
        for (const auto& port : m_outputPorts)
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
    Port* m_inputPort = nullptr;
    // A port may have multiple outputs     [port]->...->
    std::vector<Port*> m_outputPorts;

    std::function<VSRTL_VT_U()> m_propagationFunction;
    PropagationState m_propagationState = PropagationState::unpropagated;
    Component* m_parent;
    unsigned int m_width = 0;

private:
    std::string m_name;
    bool m_traversingConnection = false;
};

template <typename F, typename... Args>
void Port::traverseToRoot(const F& nodeFunc, Args&... args) {
    nodeFunc(this, args...);
    if (m_inputPort) {
        m_inputPort->traverseToRoot(nodeFunc, args...);
    }
}

template <typename F, typename... Args>
void Port::traverseToSinks(const F& nodeFunc, Args&... args) {
    nodeFunc(this, args...);
    for (const auto& p : m_outputPorts) {
        p->traverseToSinks(nodeFunc, args...);
    }
}

template <typename F, typename... Args>
void Port::traverseConnection(const F& nodeFunc, Args&... args) {
    if (m_traversingConnection)
        return;
    m_traversingConnection = true;

    nodeFunc(this, args...);
    if (m_inputPort) {
        m_inputPort->traverseConnection(nodeFunc, args...);
    }
    for (const auto& p : m_outputPorts) {
        p->traverseConnection(nodeFunc, args...);
    }

    m_traversingConnection = false;
}

}  // namespace vsrtl

#endif  // VSRTL_SIGNAL_H
