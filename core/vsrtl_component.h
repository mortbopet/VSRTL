#ifndef VSRTL_COMPONENT_H
#define VSRTL_COMPONENT_H

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
    type<__VA_ARGS__>* name = create_component<type<__VA_ARGS__>>(this)

// Non-templated subcomponent construction macro
#define SUBCOMPONENT_NT(name, type) \
private:                            \
    type* name = create_component<type>(this)

#define INPUTSIGNAL(name, bitwidth) InputSignal<bitwidth>* name = this->createInputSignal<bitwidth>(#name)
#define OUTPUTSIGNAL(name, bitwidth) OutputSignal<bitwidth>* name = createOutputSignal<bitwidth>(#name)

#define SIGNAL_VALUE(input, type) input->value<type>()

class Component {
public:
    Component(std::string displayName, Component* parent = nullptr) : m_displayName(displayName), m_parent(parent) {}
    virtual ~Component() {}

    virtual bool isRegister() const = 0;
    virtual void resetPropagation() { m_propagationState = PropagationState::unpropagated; }

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
    OutputSignal<bitwidth>* createOutputSignal(const char* name) {
        auto signal = new OutputSignal<bitwidth>(this, name);
        m_outputsignals.push_back(std::unique_ptr<OutputSignal<bitwidth>>(signal));
        return signal;
    }

    template <uint32_t bitwidth>
    InputSignal<bitwidth>* createInputSignal(const char* name) {
        InputSignal<bitwidth>* signal = new InputSignal<bitwidth>(this, name);
        m_inputsignals.push_back(std::unique_ptr<InputSignal<bitwidth>>(signal));
        return signal;
    }

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
            for (auto& component : m_subcomponents) {
                component->propagateComponent();
            }

            // Propagate outputs of the component
            for (auto& s : m_outputsignals) {
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
        for (const auto& i : m_inputsignals) {
            if (!i->isConnected()) {
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
        for (const auto& i : m_outputsignals) {
            if (!i->hasPropagationFunction()) {
                return false;
            }
        }
        return true;
    }

    const Component* getParent() const { return m_parent; }
    const std::string& getDisplayName() const { return m_displayName; }
    const std::vector<std::unique_ptr<Component>>& getSubComponents() const { return m_subcomponents; }
    const std::vector<std::unique_ptr<OutputSignalBase>>& getOutputs() const { return m_outputsignals; }
    const std::vector<std::unique_ptr<InputSignalBase>>& getInputs() const { return m_inputsignals; }
    std::vector<Component*> getInputComponents() const {
        std::vector<Component*> v;
        for (auto& s : m_inputsignals) {
            v.push_back(s->getParent());
        }
        return v;
    }

protected:
    enum class PropagationState { unpropagated, propagating, propagated };
    PropagationState m_propagationState = PropagationState::unpropagated;

    void getComponentGraph(std::map<Component*, std::vector<Component*>>& componentGraph) {
        // Register adjacent components (child components) in the graph, and add subcomponents to graph
        componentGraph[this];
        for (auto& c : m_subcomponents) {
            componentGraph[this].push_back(c.get());
            c->getComponentGraph(componentGraph);
        }
    }

protected:
    /**
     * @brief propagateInputs
     * For all registered input signals of this component, propagate the parent component of the input signal
     */

    void propagateInputs() {
        for (auto& input : m_inputsignals) {
            input->getParent()->propagateComponent();
        }
    }

    void propagateSignals() {}

    std::string m_displayName;

    Component* m_parent = nullptr;
    std::vector<std::unique_ptr<OutputSignalBase>> m_outputsignals;
    std::vector<std::unique_ptr<InputSignalBase>> m_inputsignals;
    std::vector<std::unique_ptr<Component>> m_subcomponents;
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

#endif  // VSRTL_COMPONENT_H
