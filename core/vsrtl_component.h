#ifndef VSRTL_COMPONENT_H
#define VSRTL_COMPONENT_H

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"
#include "vsrtl_port.h"

#include "Signals/Signal.h"

namespace vsrtl {

/// To allow for invokations of subcomponents with types having more than one template parameter, it is necessary to be
/// able to join the templated type, without having the preprocessor deduce the ',' in the type list as a separator in
/// the actual macro. For these cases, we may say SUBCOMPONENT(xor1, TYPE(Xor<1,2>));
#define TYPE(...) __VA_ARGS__

#define SUBCOMPONENT(name, type, ...) type* name = create_component<type>(this, #name, ##__VA_ARGS__)

#define SUBCOMPONENTS(name, type, n, ...) \
    std::vector<type*> name = create_components<type>(this, #name, n, ##__VA_ARGS__)

#define INPUTPORT(name, W) Port<W>& name = this->createInputPort<W>(#name)
#define INPUTPORTS(name, W, N) std::vector<Port<W>*> name = this->createInputPorts<W>("in", N)
#define OUTPUTPORT(name, W) Port<W>& name = createOutputPort<W>(#name)
#define OUTPUTPORTS(name, W, N) std::vector<Port<W>*> name = this->createOutputPorts<W>("in", N)

#define SIGNAL_VALUE(input, type) input.value<type>()

DefineGraphicsProxy(Component);
class Component : public Base {
public:
    Component(std::string displayName, Component* parent) : m_displayName(displayName), m_parent(parent) {}

    /**
     * @brief getBaseType
     * Used to identify the component type, which is used when determining how to draw a component. Introduced to avoid
     * intermediate base classes for many (All) components, which is a template class. For instance, it is desireable to
     * identify all instances of "Constant<...>" objects, but without introducing a "BaseConstant" class.
     * @return String identifier for the component type
     */

    virtual std::type_index getTypeId() const { return GraphicsTypeID(Component); }
    virtual bool isRegister() const { return false; }
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
    bool isPropagated() const { return m_propagationState == PropagationState::propagated; }

    /**
     * @brief addSubcomponent
     *        Adds subcomponent to the current component (this).
     *        (this) takes ownership of the component*
     * @param subcomponent
     */
    void addSubcomponent(Component* subcomponent) {
        m_subcomponents.push_back(std::unique_ptr<Component>(subcomponent));
    }

    template <unsigned int W>
    Port<W>& createInputPort(std::string name) {
        return createPort<W>(name, m_inputports);
    }
    template <unsigned int W>
    Port<W>& createOutputPort(std::string name) {
        return createPort<W>(name, m_outputports);
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

        if (isRegister()) {
            // Registers are implicitely clocked by calling propagate() on its output ports.
            /** @remark register <must> be saved before propagateComponent reaches the register ! */
            m_propagationState = PropagationState::propagated;
            for (const auto& s : m_outputports) {
                s->propagate(propagationStack);
            }
        } else {
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
                throw std::runtime_error("Component: '" + m_displayName + "' has unconnected inputs");
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

    Component* getParent() const { return m_parent; }
    const std::string& getName() const { return m_displayName; }
    const std::vector<std::unique_ptr<Component>>& getSubComponents() const { return m_subcomponents; }
    const std::vector<std::unique_ptr<PortBase>>& getOutputs() const { return m_outputports; }
    const std::vector<std::unique_ptr<PortBase>>& getInputs() const { return m_inputports; }

    Gallant::Signal0<> changed;

protected:
    template <unsigned int W>
    Port<W>& createPort(std::string name, std::vector<std::unique_ptr<PortBase>>& container) {
        Port<W>* port = new Port<W>(name, this);
        container.push_back(std::unique_ptr<Port<W>>(port));
        return *port;
    }

    template <unsigned int W>
    std::vector<Port<W>*> createPorts(std::string name, std::vector<std::unique_ptr<PortBase>>& container,
                                      unsigned int n) {
        std::vector<Port<W>*> ports;
        for (unsigned int i = 0; i < n; i++) {
            std::string i_name = name + "_" + std::to_string(i);
            Port<W>* port = new Port<W>(i_name.c_str(), this);
            container.push_back(std::unique_ptr<Port<W>>(port));
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

    mutable bool m_isVerifiedAndInitialized = false;
    std::string m_displayName;
    PropagationState m_propagationState = PropagationState::unpropagated;
    Component* m_parent = nullptr;
    std::vector<std::unique_ptr<PortBase>> m_outputports;
    std::vector<std::unique_ptr<PortBase>> m_inputports;
    std::vector<std::unique_ptr<Component>> m_subcomponents;
};

// Component object generator that registers objects in parent upon creation
template <typename T, typename... Args>
T* create_component(Component* parent, std::string name, Args... args) {
    T* ptr = new T(name, parent, args...);
    if (parent) {
        parent->addSubcomponent(static_cast<Component*>(ptr));
    }
    return ptr;
}

template <typename T, typename... Args>
std::vector<T*> create_components(Component* parent, std::string name, unsigned int n, Args... args) {
    std::vector<T*> components;
    for (unsigned int i = 0; i < n; i++) {
        std::string i_name = name + "_" + std::to_string(i);
        components.push_back(create_component<T, Args...>(parent, i_name, args...));
    }
    return components;
}

}  // namespace vsrtl

#endif  // VSRTL_COMPONENT_H
