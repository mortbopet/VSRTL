#ifndef VSRTL_SIGNAL_H
#define VSRTL_SIGNAL_H

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits.h>
#include <memory>
#include <type_traits>

#include "../interface/vsrtl_binutils.h"
#include "../interface/vsrtl_interface.h"

namespace vsrtl {
namespace core {

class Component;

enum class PropagationState { unpropagated, propagated, constant };

/**
 * @brief The PortBase class
 * Base class for ports, does not have a bit width property
 */
class PortBase : public SimPort {
public:
  PortBase(const std::string &name, SimComponent *parent, PortType type)
      : SimPort(name, parent, type) {
    assert(parent != nullptr);
  }

  bool isPropagated() const {
    return m_propagationState != PropagationState::unpropagated;
  }
  bool isConstant() const override {
    return m_propagationState == PropagationState::constant;
  }
  void resetPropagation() {
    m_propagationState = m_propagationState == PropagationState::constant
                             ? m_propagationState
                             : PropagationState::unpropagated;
  }

  virtual void propagate(std::vector<PortBase *> &propagationStack) = 0;
  virtual void propagateConstant() = 0;
  virtual void setPortValue() = 0;
  virtual bool isConnected() const = 0;

  /**
   * @brief stringValue
   * A port may define special string formatting to be displayed in the
   * graphical library. If so, owning components should set the string value
   * function to provide such values.
   */
  virtual bool isEnumPort() const override { return false; }
  virtual std::string valueToEnumString() const override {
    throw std::runtime_error("This is not an enum port!");
  }
  virtual VSRTL_VT_U enumStringToValue(const char *) const override {
    throw std::runtime_error("This is not an enum port!");
  }

protected:
  PropagationState m_propagationState = PropagationState::unpropagated;
};

template <unsigned int W>
class Port : public PortBase {
public:
  Port(const std::string &name, SimComponent *parent, PortType type)
      : PortBase(name, parent, type) {}
  bool isConnected() const override {
    return m_inputPort != nullptr || m_propagationFunction;
  }

  // Port connections are doubly linked
  void operator>>(Port<W> &toThis) {
    m_outputPorts.push_back(&toThis);
    if (toThis.m_inputPort != nullptr) {
      throw std::runtime_error(
          "Failed trying to connect port '" + getParent()->getName() + ":" +
          getName() + "' to port '" + toThis.getParent()->getName() + ":" +
          toThis.getName() + ". Port is already connected to '" +
          toThis.getInputPort()->getParent()->getName() + ":" +
          toThis.getInputPort()->getName());
    }
    toThis.m_inputPort = this;
  }

  void operator>>(const std::vector<Port<W> *> &toThis) {
    for (auto &p : toThis)
      *this >> *p;
  }

  VSRTL_VT_U uValue() const override { return m_value & generateBitmask(W); }
  VSRTL_VT_S sValue() const override { return signextend<W>(m_value); }
  unsigned int getWidth() const override { return W; }

  explicit operator VSRTL_VT_S() const { return signextend<W>(m_value); }

  void setPortValue() override {
    auto prePropagateValue = m_value;
    if (m_propagationFunction) {
      m_value = m_propagationFunction();
    } else {
      m_value = getInputPort<Port<W>>()->uValue();
    }
    if (m_value != prePropagateValue) {
      // Signal all watcher of this port that the port value changed
      if (getDesign()->signalsEnabled()) {
        changed.Emit();
      }
    }
  }

  void propagate(std::vector<PortBase *> &propagationStack) override {
    if (m_propagationState == PropagationState::unpropagated) {
      propagationStack.push_back(this);
      // Propagate the value to the ports which connect to this
      for (const auto &port : getOutputPorts<Port<W>>())
        port->propagate(propagationStack);
      m_propagationState = PropagationState::propagated;
    }
  }

  void propagateConstant() override {
    m_propagationState = PropagationState::constant;
    setPortValue();
    for (const auto &port : getOutputPorts<Port<W>>())
      port->propagateConstant();
  }

  void operator<<(std::function<VSRTL_VT_U()> &&propagationFunction) {
    if (m_propagationFunction) {
      throw std::runtime_error("Propagation function reassignment prohibited");
    }
    m_propagationFunction = propagationFunction;
  }

  // Value access operators
  explicit operator VSRTL_VT_U() const { return m_value; }
  explicit operator bool() const { return m_value & 0b1; }

protected:
  // Port values are initialized to 0xdeadbeef for error detection reasons. In
  // reality (in a circuit), this would not be the case - the entire circuit is
  // reset when the registers are reset (to 0), and the circuit state is then
  // propagated.
  VSRTL_VT_U m_value = 0xdeadbeef;

  std::function<VSRTL_VT_U()> m_propagationFunction = {};
};

template <unsigned int W, typename E_t>
class EnumPort : public Port<W> {
public:
  EnumPort(const std::string &name, Component *parent,
           vsrtl::SimPort::PortType type)
      : Port<W>(name, parent, type) {}

  bool isEnumPort() const override { return true; }
  std::string valueToEnumString() const override {
    return E_t::_from_integral(this->uValue())._to_string();
  }
  VSRTL_VT_U enumStringToValue(const char *str) const override {
    return E_t::_from_string(str);
  }
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_SIGNAL_H
