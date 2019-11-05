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

#include "vsrtl_base.h"
#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"
#include "vsrtl_port.h"

#include "Signals/Signal.h"

namespace vsrtl {

/// To allow for invokations of subcomponents with types having more than one template parameter, it is necessary to be
/// able to join the templated type, without having the preprocessor deduce the ',' in the type list as a separator in
/// the actual macro. For these cases, we may say SUBCOMPONENT(xor1, TYPE(Xor<1,2>));
#define TYPE(...) __VA_ARGS__

#define SUBCOMPONENT(name, type, ...) type* name = create_component<type>(#name, ##__VA_ARGS__)

#define SUBCOMPONENTS(name, type, n, ...) std::vector<type*> name = create_components<type>(#name, n, ##__VA_ARGS__)

#define INPUTPORT(name, W) Port<W>& name = this->createInputPort<W>(#name)
#define INPUTPORT_ENUM(name, E_t) Port<E_t::width()>& name = this->createInputPort<E_t::width(), E_t>(#name)
#define INPUTPORTS(name, W, N) std::vector<Port<W>*> name = this->createInputPorts<W>("in", N)

#define OUTPUTPORT(name, W) Port<W>& name = createOutputPort<W>(#name)
#define OUTPUTPORT_ENUM(name, E_t) Port<E_t::width()>& name = createOutputPort<E_t::width(), E_t>(#name)
#define OUTPUTPORTS(name, W, N) std::vector<Port<W>*> name = this->createOutputPorts<W>("in", N)

#define SIGNAL_VALUE(input, type) input.value<type>()

DefineGraphicsType(Component);
class Component : public Base {
public:
    Component(std::string displayName, Component* parent) : Base(displayName, parent) {}
    using PortBaseCompT = BaseSorter<std::unique_ptr<PortBase>>;
    using ComponentCompT = BaseSorter<std::unique_ptr<Component>>;

    /**
     * @brief getBaseType
     * Used to identify the component type, which is used when determining how to draw a component. Introduced to avoid
     * intermediate base classes for many (All) components, which is a template class. For instance, it is desireable to
     * identify all instances of "Constant<...>" objects, but without introducing a "BaseConstant" class.
     * @return String identifier for the component type
     */

    virtual std::type_index getTypeId() const { return GraphicsTypeFor(Component); }
    virtual bool isClockedComponent() const { return false; }
    virtual void resetPropagation() {
        if (m_propagationState == PropagationState::unpropagated) {
            // Constants (components with no inputs) are always propagated
        } else {
            m_propagationState = PropagationState::unpropagated;
            for (const auto& i : m_inputports)
                i->resetPropagation();
            for (const auto& o : m_outputports)
                o->resetPropagation();
        }
    }
    unsigned reserveConstantId() { return m_constantCount++; }
    bool isPropagated() const { return m_propagationState == PropagationState::propagated; }

    /**
     * @brief addSubcomponent
     *        Adds subcomponent to the current component (this).
     *        (this) takes ownership of the component*
     * @param subcomponent
     */
    void addSubcomponent(Component* subcomponent) { m_subcomponents.insert(std::unique_ptr<Component>(subcomponent)); }

    // Component object generator that registers objects in parent upon creation
    template <typename T, typename... Args>
    T* create_component(std::string name, Args... args) {
        verifyIsUniqueComponentName(name);
        auto* ptr = static_cast<T*>((*m_subcomponents.emplace(std::make_unique<T>(name, this, args...)).first).get());
        return ptr;
    }

    template <typename T, typename... Args>
    std::vector<T*> create_components(std::string name, unsigned int n, Args... args) {
        std::vector<T*> components;
        for (unsigned int i = 0; i < n; i++) {
            std::string i_name = name + "_" + std::to_string(i);
            components.push_back(create_component<T, Args...>(i_name, args...));
        }
        return components;
    }

    template <unsigned int W, typename E_t = void>
    Port<W>& createInputPort(std::string name) {
        return createPort<W, E_t>(name, m_inputports);
    }
    template <unsigned int W, typename E_t = void>
    Port<W>& createOutputPort(std::string name) {
        return createPort<W, E_t>(name, m_outputports);
    }

    template <unsigned int W>
    std::vector<Port<W>*> createInputPorts(std::string name, unsigned int n) {
        return createPorts<W>(name, m_inputports, n);
    }

    template <unsigned int W>
    std::vector<Port<W>*> createOutputPorts(std::string name, unsigned int n) {
        return createPorts<W>(name, m_outputports, n);
    }

    void propagateComponent(std::vector<PortBase*>& propagationStack) {
        // Component has already been propagated
        if (m_propagationState == PropagationState::propagated)
            return;

        if (isClockedComponent()) {
            // Registers are implicitely clocked by calling propagate() on its output ports.
            /** @remark register <must> be saved before propagateComponent reaches the register ! */
            m_propagationState = PropagationState::propagated;
            for (const auto& s : m_outputports) {
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

            for (const auto& sc : m_subcomponents)
                sc->propagateComponent(propagationStack);

            // All sequential logic must have their inputs propagated before they themselves can propagate. If this is
            // not the case, the function will return. Iff the circuit is correctly connected, this component will at a
            // later point be visited, given that the input port which is currently not yet propagated, will become
            // propagated at some point, signalling its connected components to propagate.
            for (const auto& input : m_inputports) {
                if (!input->isPropagated())
                    return;
            }

            for (const auto& sc : m_subcomponents)
                sc->propagateComponent(propagationStack);

            // At this point, all input ports are assured to be propagated. In this case, it is safe to propagate
            // the outputs of the component.
            for (const auto& s : m_outputports) {
                s->propagate(propagationStack);
            }
            m_propagationState = PropagationState::propagated;

            // if any internal values have changed...
            // @todo: implement granular change signal emission
            changed.Emit();
        }

        // Signal all connected components of the current component to propagate
        for (const auto& out : m_outputports) {
            for (const auto& in : out->getOutputPorts()) {
                // With the input port of the connected component propagated, the parent component may be propagated.
                // This will succeed if all input components to the parent component has been propagated.
                in->getParent()->propagateComponent(propagationStack);

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
                    inout->getParent()->propagateComponent(propagationStack);
            }
        }
    }
    void initialize() {
        if (m_inputports.size() == 0 && !hasSubcomponents()) {
            // Component has no input ports - ie. component is a constant. propagate all output ports and set component
            // as propagated.
            for (const auto& p : m_outputports)
                p->propagateConstant();
            m_propagationState = PropagationState::propagated;
        }
    }
    void verifyComponent() const {
        for (const auto& ip : m_inputports) {
            if (!ip->isConnected()) {
                throw std::runtime_error("Component: '" + getName() + "' has unconnected inputs");
            }
        }
    }
    bool hasSubcomponents() const { return m_subcomponents.size() != 0; }

    /**
     * getInput&OutputComponents does not return a set, although it naturally should. In partitioning the circuit graph,
     * it is beneficial to know whether two components have multiple edges between each other.
     */
    std::vector<Component*> getInputComponents() const {
        std::vector<Component*> v;
        for (const auto& s : m_inputports) {
            v.push_back(s->getInputPort()->getParent());
        }
        return v;
    }
    std::vector<Component*> getOutputComponents() const {
        std::vector<Component*> v;
        for (const auto& p : m_outputports) {
            for (const auto& pc : p->getOutputPorts())
                v.push_back(pc->getParent());
        }
        return v;
    }

    void verifyIsUniqueComponentName(const std::string& name) {
        if (!isUniqueName(name, m_subcomponents)) {
            throw std::runtime_error("Duplicate subcomponent name: '" + name + "' in component: '" + getName() +
                                     "'. Subcomponent names must be unique.");
        }
    }

    const std::set<std::unique_ptr<Component>, ComponentCompT>& getSubComponents() const { return m_subcomponents; }
    const std::set<std::unique_ptr<PortBase>, PortBaseCompT>& getOutputs() const { return m_outputports; }
    const std::set<std::unique_ptr<PortBase>, PortBaseCompT>& getInputs() const { return m_inputports; }

    Gallant::Signal0<> changed;

protected:
    template <unsigned int W, typename E_t = void>
    Port<W>& createPort(std::string name, std::set<std::unique_ptr<PortBase>, PortBaseCompT>& container) {
        verifyIsUniquePortName(name);
        Port<W>* port;
        if constexpr (std::is_void<E_t>::value) {
            port = static_cast<Port<W>*>((*container.emplace(std::make_unique<Port<W>>(name, this)).first).get());
        } else {
            port =
                static_cast<Port<W>*>((*container.emplace(std::make_unique<EnumPort<W, E_t>>(name, this)).first).get());
        }
        return *port;
    }

    template <unsigned int W>
    std::vector<Port<W>*> createPorts(std::string name, std::set<std::unique_ptr<PortBase>, PortBaseCompT>& container,
                                      unsigned int n) {
        std::vector<Port<W>*> ports;
        Port<W>* port;
        for (unsigned int i = 0; i < n; i++) {
            std::string i_name = name + "_" + std::to_string(i);
            verifyIsUniquePortName(i_name);
            port = static_cast<Port<W>*>(
                (*container.emplace(std::make_unique<Port<W>>(i_name.c_str(), this)).first).get());
            ports.push_back(port);
        }
        return ports;
    }

    void getComponentGraph(std::map<Component*, std::vector<Component*>>& componentGraph) {
        // Register adjacent components (child components) in the graph, and add subcomponents to graph
        componentGraph[this];
        for (const auto& c : m_subcomponents) {
            componentGraph[this].push_back(c.get());
            c->getComponentGraph(componentGraph);
        }
    }

    template <typename T, typename C_T>
    bool isUniqueName(const std::string& name, std::set<std::unique_ptr<T>, C_T>& container) {
        return std::find_if(container.begin(), container.end(),
                            [name](const auto& p) { return p->getName() == name; }) == container.end();
    }

    void verifyIsUniquePortName(const std::string& name) {
        if (!(isUniqueName(name, m_outputports) && isUniqueName(name, m_inputports))) {
            throw std::runtime_error("Duplicate port name: '" + name + "' in component: '" + getName() +
                                     "'. Port names must be unique.");
        }
    }

    unsigned m_constantCount = 0;  // Number of constants currently initialized in the component
    mutable bool m_isVerifiedAndInitialized = false;
    PropagationState m_propagationState = PropagationState::unpropagated;

    // Ports and subcomponents should be maintained as sorted sets based on port and component names, ensuring
    // consistent ordering between executions
    std::set<std::unique_ptr<PortBase>, PortBaseCompT> m_outputports;
    std::set<std::unique_ptr<PortBase>, PortBaseCompT> m_inputports;
    std::set<std::unique_ptr<Component>, ComponentCompT> m_subcomponents;
};

}  // namespace vsrtl

#endif  // VSRTL_COMPONENT_H
