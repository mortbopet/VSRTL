#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
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

#define NON_REGISTER_COMPONENT \
    void checkIfRegister() override {}

/**
 * @brief InputPair
 * maintains a relationship between a signal and the container in which it resides
 */
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
    Component(std::string displayName) : m_displayName(displayName) {}

    virtual void checkIfRegister() = 0;
    virtual void resetPropagation() { m_isPropagated = false; }

    void getComponentGraph(std::map<Component*, std::vector<Component*>>& componentGraph) {
        // Register adjacent components (child components) in the graph, and add subcomponents to graph
        componentGraph[this];
        for (auto& c : COMPONENT_CONTAINER) {
            componentGraph[this].push_back(c);
            c->getComponentGraph(componentGraph);
        }
    }

    mutable bool isVerifiedAndInitialized = false;

    void addSubcomponent(Component* subcomponent) { COMPONENT_CONTAINER.push_back(subcomponent); }

    template <uint32_t width>
    std::unique_ptr<Signal<width>> createOutputSignal() {
        auto signal = std::make_unique<Signal<width>>(this);
        OUTPUTS_CONTAINER.push_back(&*signal);
        return signal;
    }
#define OUTPUTSIGNAL(name, width) std::unique_ptr<Signal<width>> name = createOutputSignal<width>()

    template <uint32_t width>
    std::unique_ptr<Signal<width>*> createInputSignal() {
        std::unique_ptr<Signal<width>*> signal = std::make_unique<Signal<width>*>();
        INPUTS_CONTAINER.push_back(reinterpret_cast<SignalBase**>(signal.get()));
        return signal;
    }
#define INPUTSIGNAL(name, width) std::unique_ptr<Signal<width>*> name = this->createInputSignal<width>()
#define SIGNAL_VALUE(input, type) (*input)->value<type>()
    void propagateComponent() {
        checkIfRegister();
        if (!m_isPropagated) {
            propagateInputs();

            // propagate all subcomponents of the component
            for (auto component : COMPONENT_CONTAINER) {
                component->propagateComponent();
            }

            // Propagate outputs of the component
            for (auto s : OUTPUTS_CONTAINER) {
                s->propagate();
            }

            m_isPropagated = true;
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

protected:
    bool m_isPropagated = false;

private:
    /**
     * @brief propagateInputs
     * For all registered input signals of this component, propagate the parent component of the input signal
     */
    void propagateInputs() {
        for (auto input : INPUTS_CONTAINER) {
            (*input)->getParent()->propagateComponent();
        }
    }

    void propagateSignals() {}

    std::string m_displayName;

    std::vector<SignalBase*> OUTPUTS_CONTAINER;
    std::vector<SignalBase**> INPUTS_CONTAINER;
    std::vector<Component*> COMPONENT_CONTAINER;
};
}  // namespace ripes

#endif  // PRIMITIVE_H
