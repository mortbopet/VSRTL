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
#include "vsrtl_defines.h"
#include "vsrtl_port.h"
#include "vsrtl_sparsearray.h"

#include "../interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {
namespace core {

/// To allow for invokations of subcomponents with types having more than one template parameter, it is necessary to be
/// able to join the templated type, without having the preprocessor deduce the ',' in the type list as a separator in
/// the actual macro. For these cases, we may say SUBCOMPONENT(xor1, TYPE(Xor<1,2>));
#define TYPE(...) __VA_ARGS__

#define INPUTPORT(name, W) Port& name = this->template createInputPort(#name, W)
#define INPUTPORT_ENUM(name, E_t) Port& name = this->template createInputPort<E_t>(#name, E_t::width())
#define INPUTPORTS(name, W, N) std::vector<Port*> name = createInputPorts("in", W, N)

#define OUTPUTPORT(name, W) Port& name = this->template createOutputPort(#name, W)
#define OUTPUTPORT_ENUM(name, E_t) Port& name = this->template createOutputPort<E_t>(#name, E_t::width())
#define OUTPUTPORTS(name, W, N) std::vector<Port*> name = createOutputPorts("in", W, N)

/**
 * Dynamic ports
 * For components which may be dynamically instantiated with a desired port width, the dynamic ports are provided.
 * The dynamic ports must be initialized during the construction of the component. The dynamic ports must be initialized
 * before assigning signals to the port.
 */
#define DYNP_OUT(name) Port* name = (nullptr)
#define DYNP_OUT_INIT(name, W) name = &(this->template createOutputPort(#name, W))
#define DYNP_IN(name) Port* name = (nullptr)
#define DYNP_IN_INIT(name, W) name = &(this->template createInputPort(#name, W))

class Component : public SimComponent {
public:
    Component(std::string displayName, SimComponent* parent) : SimComponent(displayName, parent) {}
    /**
     * @brief getBaseType
     * Used to identify the component type, which is used when determining how to draw a component. Introduced to avoid
     * intermediate base classes for many (All) components, which is a template class. For instance, it is desireable to
     * identify all instances of "Constant<...>" objects, but without introducing a "BaseConstant" class.
     * @return String identifier for the component type
     */

    std::type_index getGraphicsID() const override { return GraphicsIDFor(Component); }
    virtual void resetPropagation() {
        if (m_propagationState == PropagationState::unpropagated) {
            // Constants (components with no inputs) are always propagated
        } else {
            m_propagationState = PropagationState::unpropagated;
            for (const auto& i : getPorts<SimPort::Direction::in, Port>())
                i->resetPropagation();
            for (const auto& o : getPorts<SimPort::Direction::out, Port>())
                o->resetPropagation();
        }
    }
    bool isPropagated() const { return m_propagationState == PropagationState::propagated; }
    void setSensitiveTo(const Port* p) { m_sensitivityList.push_back(p); }
    void setSensitiveTo(const Port& p) { setSensitiveTo(&p); }

    template <typename E_t = void>
    Port& createInputPort(std::string name, unsigned int W) {
        return createPort<E_t>(name, W, m_inputPorts);
    }
    template <typename E_t = void>
    Port& createOutputPort(std::string name, unsigned int W) {
        return createPort<E_t>(name, W, m_outputPorts);
    }

    std::vector<Port*> createInputPorts(std::string name, unsigned int W, unsigned int n) {
        return createPorts(name, W, m_inputPorts, n);
    }

    std::vector<Port*> createOutputPorts(std::string name, unsigned int W, unsigned int n) {
        return createPorts(name, W, m_outputPorts, n);
    }

    void propagateComponent(std::vector<Port*>& propagationStack) {
        // Component has already been propagated
        if (m_propagationState == PropagationState::propagated)
            return;

        if (isSynchronous()) {
            // Registers are implicitely clocked by calling propagate() on its output ports.
            /** @remark register <must> be saved before propagateComponent reaches the register ! */
            m_propagationState = PropagationState::propagated;
            for (const auto& s : getPorts<SimPort::Direction::out, Port>()) {
                s->propagate(propagationStack);
            }
        } else {
            /* A circuit should initially ask its subcomponents to propagate. Some subcomponents may be able to
             * propagate and some may not. Furthermore, This subcomponent (X) may be dependent on some of its internal
             * subcomponents to propagate.
             * Example:
             * Port Y is propagated, and now asks X to propagate.
             * X contains two subcomponents, B and A. A has all of its inputs (Y) propagated. B is reliant on 'z' to be
             * propagated. However, 'z' is dependent on A being propagated.
             *
             * 1.
             * X is trying to propagate, will initially ask its subcomponents to propagate.
             *
             * 1.
             * Asking A to propagate, will make port (h) propagated.
             * Asking B to propagate is invalid, because 'z' is unpropagated.
             *
             * 2.
             * Component X will then check whether all of its input ports are propagated (z, i). 'i' is propagated, but
             * 'z' is not, so the propagation algorithm will ask the parent component of port 'z' to propagate (C).
             *
             * 3.
             * C has one input which attaches to port 'h' of A - which is now propagated. So C may propagate, in turn
             * making 'z' propagated.
             *
             * 4.
             * All inputs have now been propagated to X. It will then again ask its subcomponents to try to
             * propagate.
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

            for (const auto& sc : getSubComponents<Component>())
                sc->propagateComponent(propagationStack);

            // All sequential logic must have their inputs propagated before they themselves can propagate. If this is
            // not the case, the function will return. Iff the circuit is correctly connected, this component will at a
            // later point be visited, given that the input port which is currently not yet propagated, will become
            // propagated at some point, signalling its connected components to propagate.
            for (const auto& input : getPorts<SimPort::Direction::in, Port>()) {
                if (!input->isPropagated())
                    return;
            }
            // Furthermore, we check whether any additional signals added to the sensitivity list are propagated.
            for (const auto& sens : m_sensitivityList) {
                if (!sens->isPropagated())
                    return;
            }

            for (const auto& sc : getSubComponents<Component>())
                sc->propagateComponent(propagationStack);

            // At this point, all input ports are assured to be propagated. In this case, it is safe to propagate
            // the outputs of the component.
            for (const auto& s : getPorts<SimPort::Direction::out, Port>()) {
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
        for (const auto& out : getPorts<SimPort::Direction::out, Port>()) {
            for (const auto& in : out->getOutputPorts()) {
                // With the input port of the connected component propagated, the parent component may be propagated.
                // This will succeed if all input components to the parent component has been propagated.
                in->getParent<Component>()->propagateComponent(propagationStack);

                // To facilitate output -> output connections, we need to trigger propagation in the output's parent
                // aswell
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
                for (const auto& inout : in->getOutputPorts())
                    inout->getParent<Component>()->propagateComponent(propagationStack);
            }
        }
    }

    void initialize() {
        if (m_inputPorts.size() == 0 && !hasSubcomponents() && m_sensitivityList.empty()) {
            // Component has no input ports - ie. component is a constant. propagate all output ports and set component
            // as propagated.
            for (const auto& p : getPorts<SimPort::Direction::out, Port>())
                p->propagateConstant();
            m_propagationState = PropagationState::propagated;
        }
    }

    virtual void verifyComponent() const {
        for (const auto& ip : getPorts<SimPort::Direction::in, Port>()) {
            if (!ip->isConnected()) {
                throw std::runtime_error("Component: '" + getName() + "' has unconnected input '" + ip->getName());
            }
        }
        for (const auto& op : getPorts<SimPort::Direction::out, Port>()) {
            if (!op->isConnected()) {
                throw std::runtime_error("Component: '" + getName() + "' has unconnected output '" + op->getName());
            }
        }
    }

protected:
    template <typename E_t = void>
    Port& createPort(std::string name, unsigned int W, std::set<std::unique_ptr<SimPort>, PortCompT>& container) {
        verifyIsUniquePortName(name);
        Port* port;
        if constexpr (std::is_void<E_t>::value) {
            port = static_cast<Port*>((*container.emplace(std::make_unique<Port>(name, W, this)).first).get());
        } else {
            port = static_cast<Port*>((*container.emplace(std::make_unique<EnumPort<E_t>>(name, W, this)).first).get());
        }
        return *port;
    }

    std::vector<Port*> createPorts(std::string name, unsigned int W,
                                   std::set<std::unique_ptr<SimPort>, PortCompT>& container, unsigned int n) {
        std::vector<Port*> ports;
        Port* port;
        for (unsigned int i = 0; i < n; i++) {
            std::string i_name = name + "_" + std::to_string(i);
            verifyIsUniquePortName(i_name);
            port =
                static_cast<Port*>((*container.emplace(std::make_unique<Port>(i_name.c_str(), W, this)).first).get());
            ports.push_back(port);
        }
        return ports;
    }

    void verifyIsUniquePortName(const std::string& name) {
        if (!(isUniqueName(name, m_outputPorts) && isUniqueName(name, m_inputPorts))) {
            throw std::runtime_error("Duplicate port name: '" + name + "' in component: '" + getName() +
                                     "'. Port names must be unique.");
        }
    }

    std::vector<const Port*> m_sensitivityList;
    PropagationState m_propagationState = PropagationState::unpropagated;
};

}  // namespace core
}  // namespace vsrtl

#endif  // VSRTL_COMPONENT_H
