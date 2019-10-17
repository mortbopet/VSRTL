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

/**
 * @brief The PortBase class
 * Base class for ports, does not have a bit width property
 */
class PortBase : public Base {
public:
    PortBase(std::string name, Component* parent) : m_name(name), m_parent(parent) { assert(parent != nullptr); }

    std::string getName() const { return m_name; }
    Component* getParent() const { return m_parent; }
    bool isPropagated() const { return m_propagationState != PropagationState::unpropagated; }
    void resetPropagation() {
        m_propagationState =
            m_propagationState == PropagationState::constant ? m_propagationState : PropagationState::unpropagated;
    }

    /* traverse from any given port towards its root (source) port, while executing nodeFunc(args...) in each port which
    is visited along the way*/
    template <typename F, typename... Args>
    void traverseToRoot(const F& nodeFunc, Args&... args) {
        nodeFunc(this, args...);
        if (getInputPort()) {
            getInputPort()->traverseToRoot(nodeFunc, args...);
        }
    }

    /* From this port, visit all directly and implicitely connected port to this port, and execute nodeFunc(args...) in
    each visited port */
    template <typename F, typename... Args>
    void traverseConnection(const F& nodeFunc, Args&... args) {
        if (m_traversingConnection)
            return;
        m_traversingConnection = true;

        nodeFunc(this, args...);
        if (getInputPort()) {
            getInputPort()->traverseConnection(nodeFunc, args...);
        }
        for (const auto& p : getOutputPorts()) {
            p->traverseConnection(nodeFunc, args...);
        }

        m_traversingConnection = false;
    }

    /* Traverse from any given port towards its endpoint sinks, executing nodeFunc(args...) in each visited port */
    template <typename F, typename... Args>
    void traverseToSinks(const F& nodeFunc, Args&... args) {
        nodeFunc(this, args...);
        for (const auto& p : getOutputPorts()) {
            p->traverseToSinks(nodeFunc, args...);
        }
    }

    std::vector<PortBase*> getPortsInConnection() {
        std::vector<PortBase*> portsInConnection;
        traverseConnection([=](PortBase* port, std::vector<PortBase*>& ports) { ports.push_back(port); },
                           portsInConnection);
        return portsInConnection;
    }

    virtual unsigned int getWidth() = 0;
    virtual VSRTL_VT_U uValue() = 0;
    virtual VSRTL_VT_S sValue() = 0;
    virtual std::vector<PortBase*> getOutputPorts() = 0;
    virtual PortBase* getInputPort() = 0;
    virtual void propagate(std::vector<PortBase*>& propagationStack) = 0;
    virtual void propagateConstant() = 0;
    virtual void setPortValue() = 0;
    virtual bool isConnected() const = 0;

    Gallant::Signal0<> changed;

protected:
    PropagationState m_propagationState = PropagationState::unpropagated;
    std::string m_name;
    Component* m_parent;
    bool m_traversingConnection = false;
};

template <unsigned int W>
class Port : public PortBase {
public:
    Port(std::string name, Component* parent) : PortBase(name, parent) {}
    bool isConnected() const override { return m_inputPort != nullptr || m_propagationFunction != nullptr; }

    std::vector<PortBase*> getOutputPorts() override {
        std::vector<PortBase*> ports;
        for (const auto& port : m_outputPorts) {
            ports.push_back(port);
        }
        return ports;
    }

    PortBase* getInputPort() { return m_inputPort; }

    // Port connections are doubly linked
    void operator>>(Port<W>& toThis) {
        m_outputPorts.push_back(&toThis);
        if (toThis.m_inputPort != nullptr) {
            throw std::runtime_error("Port input already connected");
        }
        toThis.m_inputPort = this;
    }

    void operator>>(const std::vector<Port<W>*>& toThis) {
        for (auto& p : toThis)
            *this >> *p;
    }

    template <typename T>
    T value() {
        return static_cast<T>(signextend<T, W>(m_value));
    }

    VSRTL_VT_U uValue() override { return value<VSRTL_VT_U>(); }
    VSRTL_VT_S sValue() override { return value<VSRTL_VT_S>(); }
    unsigned int getWidth() override { return W; }

    explicit operator VSRTL_VT_S() const { return signextend<VSRTL_VT_S, W>(m_value); }

    void setPortValue() override {
        auto prePropagateValue = m_value;
        if (m_propagationFunction != nullptr) {
            m_value = m_propagationFunction();
        } else {
            m_value = m_inputPort->template value<VSRTL_VT_U>();
        }
        if (m_value != prePropagateValue) {
            // Signal all watcher of this port that the port value changed
            changed.Emit();
        }
    }

    void propagate(std::vector<PortBase*>& propagationStack) override {
        if (m_propagationState == PropagationState::unpropagated) {
            propagationStack.push_back(this);
            // Propagate the value to the ports which connect to this
            for (const auto& port : m_outputPorts)
                port->propagate(propagationStack);
            m_propagationState = PropagationState::propagated;
        }
    }

    void propagateConstant() override {
        m_propagationState = PropagationState::constant;
        setPortValue();
        for (const auto& port : m_outputPorts)
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

protected:
    // Port values are initialized to 0xdeadbeef for error detection reasons. In reality (in a circuit), this would not
    // be the case - the entire circuit is reset when the registers are reset (to 0), and the circuit state is then
    // propagated.
    VSRTL_VT_U m_value = 0xdeadbeef;

    // A port may only have a single input  ->[port]
    Port<W>* m_inputPort = nullptr;
    // A port may have multiple outputs     [port]->...->
    std::vector<Port<W>*> m_outputPorts;

    std::function<VSRTL_VT_U()> m_propagationFunction;
};

}  // namespace vsrtl

#endif  // VSRTL_SIGNAL_H
