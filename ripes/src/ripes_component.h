#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ripes_binutils.h"
#include "ripes_defines.h"
#include "ripes_signal.h"

/**
 * @brief The Primitive class
 * A primitive for all hardware components - describes the node structure in the datapath graph.
 */

namespace ripes {

#define COMPONENT_CONTAINER _components
#define INPUTS_CONTAINER _inputs
#define OUTPUTS_CONTAINER _outputs

/**
 * @brief InputPair
 * maintains a relationship between a signal and the container in which it resides
 */
typedef std::pair<Component*, SignalBase**> InputPair;

class Component;

// Component object generator that registers objects in parent upon creation
template <typename T, typename... Args>
std::unique_ptr<T> create_component(Component* parent, Args&&... args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    if (parent) {
        parent->addSubcomponent(static_cast<Component*>(ptr.get()));
    }
    return ptr;
}

#define SUBCOMPONENT(name, type, ...) \
    std::unique_ptr<type<__VA_ARGS__>> name = create_component<type<__VA_ARGS__>>(this)

class Component {
public:
    Component(std::string displayName = "unnamed") : m_displayName(displayName) {}
    /**
     * @brief verify
     * Ensures that all inputs are well defined
     */

    virtual void resetPropagation() { m_isPropagated = false; }

    void verifyComponent() const {
        verifySubcomponents();
        /*
        for (const auto& p : m_inputs) {
        // Verify that all inputs are defined and that the parent containers of the inputs are set (not nullptr)
            ASSERT_CONNECTION_DEFINED(p)
        }
        */
        isVerifiedAndInitialized = true;
        std::cout << "IMPLEMENT ME!" << std::endl;
    }

    /**
     * @brief verifyDesign
     * Calls verify() on each subcomponent withing the component and checks for combinational loops
     */
    void verifySubcomponents() const { /** @todo check for combinational loops */
        for (const auto& component : COMPONENT_CONTAINER) {
            component->verifyComponent();
        }
    }
    mutable bool isVerifiedAndInitialized = false;

    void addSubcomponent(Component* subcomponent) { COMPONENT_CONTAINER.push_back(subcomponent); }

    template <uint32_t width>
    std::unique_ptr<Signal<width>> createOutputSignal() {
        auto signal = std::make_unique<Signal<width>>();
        OUTPUTS_CONTAINER.push_back(&*signal);
        return signal;
    }
#define OUTPUTSIGNAL(name, width) std::unique_ptr<Signal<width>> name = createOutputSignal<width>()

    template <uint32_t width>
    std::unique_ptr<Signal<width>*> createInputSignal() {
        auto signal = std::make_unique<Signal<width>*>();
        InputPair p =
            std::make_pair(reinterpret_cast<Component*>(nullptr), reinterpret_cast<SignalBase**>(signal.get()));
        INPUTS_CONTAINER.push_back(p);
        return signal;
    }
#define INPUTSIGNAL(name, width) std::unique_ptr<Signal<width>*> name = this->createInputSignal<width>()
#define SIGNAL_VALUE(input, type) (*input)->value<type>()
    void propagateComponent() {
        if (!m_isPropagated) {
            propagateInputs();

            // Inputs are propagated, propagate top-level outputs of the component

            // propagate all subcomponents of the component
            for (auto component : COMPONENT_CONTAINER) {
                component->propagateComponent();
            }

            // Reset propagation flag of all components & signals
            for (auto component : COMPONENT_CONTAINER) {
                component->resetPropagation();
            }

            m_isPropagated = true;
        }
    }

protected:
    bool m_isPropagated = false;

private:
    /**
     * @brief propagateInputs
     * For all registered input signals of this component, propagate the parent component of the input signal
     */
    void propagateInputs() {
        for (auto input : INPUTS_CONTAINER) {
            input.first->propagateComponent();
        }
    }

    void propagateSignals() {}

    std::string m_displayName;

    std::vector<SignalBase*> OUTPUTS_CONTAINER;
    std::vector<InputPair> INPUTS_CONTAINER;
    std::vector<Component*> COMPONENT_CONTAINER;
};
}  // namespace ripes

#endif  // PRIMITIVE_H
