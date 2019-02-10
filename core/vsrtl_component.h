#ifndef VSRTL_COMPONENT_H
#define VSRTL_COMPONENT_H

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"
#include "vsrtl_port.h"

/**
 * @brief The Primitive class
 * A primitive for all hardware components - describes the node structure in the datapath graph.
 */

namespace vsrtl {

#define NON_REGISTER_COMPONENT \
    bool isRegister() const override { return false; }

#define REGISTER_COMPONENT \
    bool isRegister() const override { return true; }

/** @class Component
 *
 *  @note: Signals:
 *         output signals are owned by components (as unique pointers) wherein
 *         input signals are pointers to output signals.
 */
class Component;

#define SUBCOMPONENT(name, type, ...) \
private:                              \
    type<__VA_ARGS__>* name = create_component<type<__VA_ARGS__>>(this, #name)

// Non-templated subcomponent construction macro
#define SUBCOMPONENT_NT(name, type) \
private:                            \
    type* name = create_component<type>(this, #name)

#define INPUTPORT(name, bitwidth) Port<bitwidth>& name = this->createInputPort<bitwidth>(#name)
#define INPUTPORTS(name, bitwidth, n) std::vector<Port<bitwidth>*> name = this->createInputPorts<bitwidth>(#name, n)
#define OUTPUTPORT(name, bitwidth) Port<bitwidth>& name = createOutputPort<bitwidth>(#name)

#define SIGNAL_VALUE(input, type) input.value<type>()

class Component {
public:
    Component(std::string displayName, Component* parent = nullptr) : m_displayName(displayName), m_parent(parent) {}
    virtual ~Component() {}

    /**
     * @brief getBaseType
     * Used to identify the component type, which is used when determining how to draw a component. Introduced to avoid
     * intermediate base classes for many (All) components, which is a template class. For instance, it is desireable to
     * identify all instances of "Constant<...>" objects, but without introducing a "BaseConstant" class.
     * @return String identifier for the component type
     */
    virtual std::string getBaseType() const { return "Component"; }

    virtual bool isRegister() const = 0;
    virtual void resetPropagation() {
        if (m_propagationState == PropagationState::unpropagated) {
            // Constants (components with no inputs) are always propagated
        } else {
            m_propagationState = PropagationState::unpropagated;
            for (auto& i : m_inputports)
                i->resetPropagation();
            for (auto& o : m_outputports)
                o->resetPropagation();
        }
    }
    bool isPropagated() const { return m_propagationState == PropagationState::propagated; }

    mutable bool m_isVerifiedAndInitialized = false;

    /**
     * @brief addSubcomponent
     *        Adds subcomponent to the current component (this).
     *        (this) takes ownership of the component*
     * @param subcomponent
     */
    void addSubcomponent(Component* subcomponent) {
        m_subcomponents.push_back(std::unique_ptr<Component>(subcomponent));
    }

    template <uint32_t bitwidth>
    Port<bitwidth>& createOutputPort(std::string name) {
        auto port = new Port<bitwidth>(name, this);
        m_outputports.push_back(std::unique_ptr<Port<bitwidth>>(port));
        return *port;
    }

    template <uint32_t bitwidth>
    Port<bitwidth>& createInputPort(std::string name) {
        Port<bitwidth>* port = new Port<bitwidth>(name, this);
        m_inputports.push_back(std::unique_ptr<Port<bitwidth>>(port));
        return *port;
    }

    template <uint32_t bitwidth>
    std::vector<Port<bitwidth>*> createInputPorts(std::string name, unsigned int n) {
        std::vector<Port<bitwidth>*> ports;
        for (int i = 0; i < n; i++) {
            std::string i_name = name + "_" + std::to_string(i);
            Port<bitwidth>* port = new Port<bitwidth>(i_name.c_str(), this);
            m_inputports.push_back(std::unique_ptr<Port<bitwidth>>(port));
            ports.push_back(port);
        }
        return ports;
    }

    void propagateComponent() {
        // Component has already been propagated
        if (m_propagationState == PropagationState::propagated)
            return;

        if (isRegister()) {
            // Registers have been propagated in the clocking action
            m_propagationState = PropagationState::propagated;
        } else {
            // All sequential logic must have their inputs propagated before they themselves can propagate. If this is
            // not the case, the function will return. Iff the circuit is correctly connected, this component will at a
            // later point be visited, given that the input port which is currently not yet propagated, will become
            // propagated at some point, signalling its connected components to propagate.
            for (const auto& input : m_inputports) {
                if (!input->isPropagated())
                    return;
            }

            for (auto& sc : m_subcomponents)
                sc->propagateComponent();

            // At this point, all input ports are assured to be propagated. In this case, it is safe to propagate
            // the outputs of the component.
            for (auto& s : m_outputports) {
                s->propagate();
            }
            m_propagationState = PropagationState::propagated;
        }

        // Signal all connected components of the current component to propagate
        for (auto& out : m_outputports) {
            for (auto& in : out->getConnectsFromThis()) {
                // With the input port of the connected component propagated, the parent component may be propagated.
                // This will succeed if all input components to the parent component has been propagated.
                in->getParent()->propagateComponent();

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
                for (auto& inout : in->getConnectsFromThis())
                    inout->getParent()->propagateComponent();
            }
        }
    }

    void verifyComponent() const {
        for (const auto& ip : m_inputports) {
            if (!ip->isConnected()) {
                throw std::runtime_error("A component has unconnected inputs");
            }
        }
    }
    void initialize() {
        if (m_inputports.size() == 0) {
            // Component has no input ports - ie. component is a constant. propagate all output ports and set component
            // as propagated.
            for (auto& p : m_outputports)
                p->propagateConstant();
            m_propagationState = PropagationState::propagated;
        }
    }

    const Component* getParent() const { return m_parent; }
    std::string getName() const { return m_displayName; }
    const std::vector<std::unique_ptr<Component>>& getSubComponents() const { return m_subcomponents; }
    const std::vector<std::unique_ptr<PortBase>>& getOutputs() const { return m_outputports; }
    const std::vector<std::unique_ptr<PortBase>>& getInputs() const { return m_inputports; }
    std::vector<Component*> getInputComponents() const {
        std::vector<Component*> v;
        for (auto& s : m_inputports) {
            v.push_back(s->getParent());
        }
        return v;
    }

    std::vector<Component*> getOutputComponents() const {
        std::vector<Component*> v;
        for (auto& p : m_outputports) {
            for (auto& pc : p->getConnectsFromThis())
                v.push_back(pc->getParent());
        }
        return v;
    }

protected:
    PropagationState m_propagationState = PropagationState::unpropagated;

    void getComponentGraph(std::map<Component*, std::vector<Component*>>& componentGraph) {
        // Register adjacent components (child components) in the graph, and add subcomponents to graph
        componentGraph[this];
        for (auto& c : m_subcomponents) {
            componentGraph[this].push_back(c.get());
            c->getComponentGraph(componentGraph);
        }
    }

    std::string m_displayName;

    Component* m_parent = nullptr;
    std::vector<std::unique_ptr<PortBase>> m_outputports;
    std::vector<std::unique_ptr<PortBase>> m_inputports;
    std::vector<std::unique_ptr<Component>> m_subcomponents;
};  // namespace vsrtl

// Component object generator that registers objects in parent upon creation
template <typename T, typename... Args>
T* create_component(Component* parent, std::string name, Args&&... args) {
    auto ptr = new T(name, std::forward<Args>(args)...);
    if (parent) {
        parent->addSubcomponent(static_cast<Component*>(ptr));
    }
    return ptr;
}

}  // namespace vsrtl

#endif  // VSRTL_COMPONENT_H
