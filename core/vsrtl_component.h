#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"
#include "vsrtl_signal.h"

/**
 * @brief The Primitive class
 * A primitive for all hardware components - describes the node structure in the datapath graph.
 */

namespace vsrtl {

#define COMPONENT_CONTAINER _components
#define INPUTS_CONTAINER _inputs
#define OUTPUTS_CONTAINER _outputs

#define NON_REGISTER_COMPONENT \
    bool isRegister() override { return false; }

#define REGISTER_COMPONENT \
    bool isRegister() override { return true; }

/**
 * @brief InputPair
 * maintains a relationship between a signal and the container in which it resides
 */
class Component;

#define SUBCOMPONENT(name, type, ...) \
private:                              \
    type<__VA_ARGS__>* name = create_component<type<__VA_ARGS__>>(this)

// Non-templated subcomponent construction macro
#define SUBCOMPONENT_NT(name, type) \
private:                            \
    type* name = create_component<type>(this)

class Component {
public:
    Component(std::string displayName, Component* parent = nullptr) : m_displayName(displayName), m_parent(parent) {}

    virtual bool isRegister() = 0;
    virtual void resetPropagation() { m_propagationState = PropagationState::unpropagated; }

    mutable bool isVerifiedAndInitialized = false;

    void addSubcomponent(Component* subcomponent) { COMPONENT_CONTAINER.push_back(subcomponent); }

    template <uint32_t width>
    Signal<width>* createOutputSignal() {
        auto signal = new Signal<width>(this);
        OUTPUTS_CONTAINER.push_back(signal);
        return signal;
    }

#define OUTPUTSIGNAL(name, width) Signal<width>* name = createOutputSignal<width>()

    template <uint32_t width>
    Signal<width>*** createInputSignal() {
        Signal<width>*** signal = new Signal<width>**();
        *signal = new Signal<width>*();
        **signal = nullptr;
        INPUTS_CONTAINER.push_back(reinterpret_cast<SignalBase***>(signal));
        return signal;
    }
#define INPUTSIGNAL(name, width) Signal<width>*** name = this->createInputSignal<width>()
#define SIGNAL_VALUE(input, type) (*(*input))->template value<type>()
    void propagateComponent() {
        if (m_propagationState == PropagationState::unpropagated) {
            // if subcomponents are connected together, propapagation can look like a combinational loop
            // Below flag ensures that propagation is only started once for a component.
            // Checking for combinational loop? hmm..
            m_propagationState = PropagationState::propagating;
            /** @note Registers do NOT require propagated inputs (these have been implicitely propagated during the
             * last cycle) This is the logic which helps simulate digital electronics
             *
             */
            if (!isRegister()) {
                propagateInputs();
            }

            // propagate all subcomponents of the component
            for (auto component : COMPONENT_CONTAINER) {
                component->propagateComponent();
            }

            // Propagate outputs of the component
            for (auto s : OUTPUTS_CONTAINER) {
                s->propagate();
            }

            m_propagationState = PropagationState::propagated;
        } else {
            // throw std::runtime_error("Combinational loop detected");
        }
    }

    /**
     * @brief verifyInputs
     * Checks whether all inputs have been fully specified (connected)
     * @return
     */
    bool verifyInputs() {
        for (const auto& i : INPUTS_CONTAINER) {
            if (*i == nullptr) {
                return false;
            }
        }
        return true;
    }
    /**
     * @brief verifyOutputs
     * Verifies that all outputs of this component has a propagation function
     * @return
     */
    bool verifyOutputs() {
        for (const auto& i : OUTPUTS_CONTAINER) {
            if (!i->hasPropagationFunction()) {
                return false;
            }
        }
        return true;
    }

    const Component* const& getParent() const { return m_parent; }
    const std::string& getDisplayName() const { return m_displayName; }
    const std::vector<Component*>& getSubComponents() const { return COMPONENT_CONTAINER; }
    const std::vector<SignalBase*>& getOutputs() const { return OUTPUTS_CONTAINER; }
    const std::vector<SignalBase***>& getInputs() const { return INPUTS_CONTAINER; }
    std::vector<Component*> getInputComponents() const {
        std::vector<Component*> v;
        for (auto& s : INPUTS_CONTAINER) {
            v.push_back((**s)->getParent());
        }
        return v;
    }

protected:
    enum class PropagationState { unpropagated, propagating, propagated };
    PropagationState m_propagationState = PropagationState::unpropagated;

    void getComponentGraph(std::map<Component*, std::vector<Component*>>& componentGraph) {
        // Register adjacent components (child components) in the graph, and add subcomponents to graph
        componentGraph[this];
        for (auto& c : COMPONENT_CONTAINER) {
            componentGraph[this].push_back(c);
            c->getComponentGraph(componentGraph);
        }
    }

protected:
    /**
     * @brief propagateInputs
     * For all registered input signals of this component, propagate the parent component of the input signal
     */

    void propagateInputs() {
        for (auto input : INPUTS_CONTAINER) {
            (*(*input))->getParent()->propagateComponent();
        }
    }

    void propagateSignals() {}

    std::string m_displayName;

    Component* m_parent = nullptr;
    std::vector<SignalBase*> OUTPUTS_CONTAINER;
    std::vector<SignalBase***> INPUTS_CONTAINER;
    std::vector<Component*> COMPONENT_CONTAINER;
};

// Component object generator that registers objects in parent upon creation
template <typename T, typename... Args>
T* create_component(Component* parent, Args&&... args) {
    auto ptr = new T(std::forward<Args>(args)...);
    if (parent) {
        parent->addSubcomponent(static_cast<Component*>(ptr));
    }
    return ptr;
}

}  // namespace vsrtl

#endif  // PRIMITIVE_H
