#ifndef VSRTL_COMPONENT_H
#define VSRTL_COMPONENT_H

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "../interface/vsrtl_binutils.h"
#include "../interface/vsrtl_defines.h"
#include "vsrtl_addressspace.h"
#include "vsrtl_port.h"

#include "../interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {
namespace core {

/// To allow for invokations of subcomponents with types having more than one
/// template parameter, it is necessary to be able to join the templated type,
/// without having the preprocessor deduce the ',' in the type list as a
/// separator in the actual macro. For these cases, we may say
/// SUBCOMPONENT(xor1, TYPE(Xor<1,2>));
#define TYPE(...) __VA_ARGS__

#define INPUTPORT(name, W)                                                     \
  Port<W> &name = this->template createInputPort<W>(#name)
#define INPUTPORT_ENUM(name, E_t)                                              \
  Port<E_t::width()> &name =                                                   \
      this->template createInputPort<E_t::width(), E_t>(#name)
#define INPUTPORTS(name, W, N)                                                 \
  std::vector<Port<W> *> name = this->template createInputPorts<W>("in", N)

#define OUTPUTPORT(name, W)                                                    \
  Port<W> &name = this->template createOutputPort<W>(#name)
#define OUTPUTPORT_ENUM(name, E_t)                                             \
  Port<E_t::width()> &name =                                                   \
      this->template createOutputPort<E_t::width(), E_t>(#name)
#define OUTPUTPORTS(name, W, N)                                                \
  std::vector<Port<W> *> name = this->template createOutputPorts<W>("in", N)

class Component : public SimComponent {
public:
  Component(const std::string &displayName, SimComponent *parent)
      : SimComponent(displayName, parent) {}
  /**
   * @brief getBaseType
   * Used to identify the component type, which is used when determining how to
   * draw a component. Introduced to avoid intermediate base classes for many
   * (All) components, which is a template class. For instance, it is desireable
   * to identify all instances of "Constant<...>" objects, but without
   * introducing a "BaseConstant" class.
   * @return String identifier for the component type
   */

  const GraphicsType *getGraphicsType() const override {
    return GraphicsTypeFor(Component);
  }
  virtual void resetPropagation() {
    if (m_propagationState == PropagationState::unpropagated) {
      // Constants (components with no inputs) are always propagated
    } else {
      m_propagationState = PropagationState::unpropagated;
      for (const auto &i : getPorts<SimPort::PortType::in, PortBase>())
        i->resetPropagation();
      for (const auto &o : getPorts<SimPort::PortType::out, PortBase>())
        o->resetPropagation();
    }
  }
  bool isPropagated() const {
    return m_propagationState == PropagationState::propagated;
  }
  void setSensitiveTo(const PortBase *p) { m_sensitivityList.push_back(p); }
  void setSensitiveTo(const PortBase &p) { setSensitiveTo(&p); }

  template <unsigned int W, typename E_t = void>
  Port<W> &createInputPort(const std::string &name) {
    return createPort<W, E_t>(name, m_inputPorts, vsrtl::SimPort::PortType::in);
  }
  template <unsigned int W, typename E_t = void>
  Port<W> &createOutputPort(const std::string &name) {
    return createPort<W, E_t>(name, m_outputPorts,
                              vsrtl::SimPort::PortType::out);
  }

  template <unsigned int W>
  std::vector<Port<W> *> createInputPorts(const std::string &name,
                                          unsigned int n) {
    return createPorts<W>(name, m_inputPorts, vsrtl::SimPort::PortType::in, n);
  }

  template <unsigned int W>
  std::vector<Port<W> *> createOutputPorts(const std::string &name,
                                           unsigned int n) {
    return createPorts<W>(name, m_outputPorts, vsrtl::SimPort::PortType::out,
                          n);
  }

  void propagateComponent(std::vector<PortBase *> &propagationStack) {
    // Component has already been propagated
    if (m_propagationState == PropagationState::propagated)
      return;

    if (isSynchronous()) {
      // Registers are implicitely clocked by calling propagate() on its output
      // ports.
      /** @remark register <must> be saved before propagateComponent reaches the
       * register ! */
      m_propagationState = PropagationState::propagated;
      for (const auto &s : getPorts<SimPort::PortType::out, PortBase>()) {
        s->propagate(propagationStack);
      }
    } else {
      /* A circuit should initially ask its subcomponents to propagate. Some
       * subcomponents may be able to propagate and some may not. Furthermore,
       * This subcomponent (X) may be dependent on some of its internal
       * subcomponents to propagate.
       * Example:
       * Port Y is propagated, and now asks X to propagate.
       * X contains two subcomponents, B and A. A has all of its inputs (Y)
       * propagated. B is reliant on 'z' to be propagated. However, 'z' is
       * dependent on A being propagated.
       *
       * 1.
       * X is trying to propagate, will initially ask its subcomponents to
       * propagate.
       *
       * 1.
       * Asking A to propagate, will make port (h) propagated.
       * Asking B to propagate is invalid, because 'z' is unpropagated.
       *
       * 2.
       * Component X will then check whether all of its input ports are
       * propagated (z, i). 'i' is propagated, but 'z' is not, so the
       * propagation algorithm will ask the parent component of port 'z' to
       * propagate (C).
       *
       * 3.
       * C has one input which attaches to port 'h' of A - which is now
       * propagated. So C may propagate, in turn making 'z' propagated.
       *
       * 4.
       * All inputs have now been propagated to X. It will then again ask its
       * subcomponents to try to propagate.
       *
       * 5.
       *
       *  y
       *  +                 X
       *  |          +--------------+
       *  |          |   +------+   |
       *  |          |   |      |   |
       *  |          |   |  B   |   |
       *  |      z   |   |      |   |        +------+
       *  |    +---->---->      |   |        |      |
       *  |    |     |   +------+   |        |      |
       *  |    |     |              |    +--->  C   +----+
       *  |    |     |   +------+   |    |   |      |    |
       *  |    |  i  |   |      | h |    |   +------+    |
       *  +--------->---->  A   +--------+               |
       *       |     |   |      |   |                    |
       *       |     |   |      |   |                    |
       *       |     |   +------+   |                    |
       *       |     |              |                    |
       *       |     +--------------+                    |
       *       |                                         |
       *       +-----------------------------------------+
       *
       */

      for (const auto &sc : getSubComponents<Component>())
        sc->propagateComponent(propagationStack);

      // All sequential logic must have their inputs propagated before they
      // themselves can propagate. If this is not the case, the function will
      // return. Iff the circuit is correctly connected, this component will at
      // a later point be visited, given that the input port which is currently
      // not yet propagated, will become propagated at some point, signalling
      // its connected components to propagate.
      for (const auto &input : getPorts<SimPort::PortType::in, PortBase>()) {
        if (!input->isPropagated())
          return;
      }
      // Furthermore, we check whether any additional signals added to the
      // sensitivity list are propagated.
      for (const auto &sens : m_sensitivityList) {
        if (!sens->isPropagated())
          return;
      }

      for (const auto &sc : getSubComponents<Component>())
        sc->propagateComponent(propagationStack);

      // At this point, all input ports are assured to be propagated. In this
      // case, it is safe to propagate the outputs of the component.
      for (const auto &s : getPorts<SimPort::PortType::out, PortBase>()) {
        s->propagate(propagationStack);
      }
      m_propagationState = PropagationState::propagated;

      // if any internal values have changed...
      // @todo: implement granular change signal emission
      if (getDesign()->signalsEnabled()) {
        changed.Emit();
      }
    }

    // Signal all connected components of the current component to propagate
    for (const auto &out : getPorts<SimPort::PortType::out, PortBase>()) {
      for (const auto &in : out->getOutputPorts()) {
        // With the input port of the connected component propagated, the parent
        // component may be propagated. This will succeed if all input
        // components to the parent component has been propagated.
        in->getParent<Component>()->propagateComponent(propagationStack);

        // To facilitate output -> output connections, we need to trigger
        // propagation in the output's parent aswell
        /**
         * IN   IN   OUT  OUT
         *   _____________
         *  |    _____   |
         *  |   |    |   |
         *  |   |   ->--->
         *  |   |____|   |
         *  |____________|
         *
         */
        for (const auto &inout : in->getOutputPorts())
          inout->getParent<Component>()->propagateComponent(propagationStack);
      }
    }
  }

  void initialize() {
    if (m_inputPorts.size() == 0 && !hasSubcomponents() &&
        m_sensitivityList.empty()) {
      // Component has no input ports - ie. component is a constant. propagate
      // all output ports and set component as propagated.
      for (const auto &p : getPorts<SimPort::PortType::out, PortBase>())
        p->propagateConstant();
      m_propagationState = PropagationState::propagated;
    }
  }

  virtual void verifyComponent() const {
    for (const auto &ip : getPorts<SimPort::PortType::in, PortBase>()) {
      if (!ip->isConnected()) {
        throw std::runtime_error("Component: '" + getName() +
                                 "' has unconnected input '" + ip->getName());
      }
    }
    for (const auto &op : getPorts<SimPort::PortType::out, PortBase>()) {
      if (!op->isConnected()) {
        throw std::runtime_error("Component: '" + getName() +
                                 "' has unconnected output '" + op->getName());
      }
    }
  }

protected:
  template <unsigned int W, typename E_t = void>
  Port<W> &
  createPort(const std::string &name,
             std::set<std::unique_ptr<SimPort>, PortBaseCompT> &container,
             vsrtl::SimPort::PortType type) {
    verifyIsUniquePortName(name);
    Port<W> *port;
    if constexpr (std::is_void<E_t>::value) {
      port = static_cast<Port<W> *>(
          (*container.emplace(std::make_unique<Port<W>>(name, this, type))
                .first)
              .get());
    } else {
      port = static_cast<Port<W> *>(
          (*container
                .emplace(std::make_unique<EnumPort<W, E_t>>(name, this, type))
                .first)
              .get());
    }
    return *port;
  }

  template <unsigned int W>
  std::vector<Port<W> *>
  createPorts(const std::string &name,
              std::set<std::unique_ptr<SimPort>, PortBaseCompT> &container,
              vsrtl::SimPort::PortType type, unsigned int n) {
    std::vector<Port<W> *> ports;
    Port<W> *port;
    for (unsigned int i = 0; i < n; i++) {
      std::string i_name = name + "_" + std::to_string(i);
      verifyIsUniquePortName(i_name);
      port = static_cast<Port<W> *>(
          (*container
                .emplace(std::make_unique<Port<W>>(i_name.c_str(), this, type))
                .first)
              .get());
      ports.push_back(port);
    }
    return ports;
  }

  std::vector<const PortBase *> m_sensitivityList;
  PropagationState m_propagationState = PropagationState::unpropagated;
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_COMPONENT_H
