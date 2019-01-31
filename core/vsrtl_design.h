#ifndef VSRTL_DESIGN_H
#define VSRTL_DESIGN_H

#include "vsrtl_component.h"
#include "vsrtl_defines.h"
#include "vsrtl_memory.h"
#include "vsrtl_register.h"

#include <memory>
#include <set>
#include <type_traits>
#include <utility>

namespace vsrtl {

/**
 * @brief The Design class
 * superclass for all Design descriptions
 */

class Design : public Component {
    NON_REGISTER_COMPONENT
public:
    Design(const char* name) : Component(name) {}

    /*
    template <typename T, typename... Args>
    std::shared_ptr<T> create(Args&&... args) {
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        this->m_components.push_back(ptr);
        if (std::is_base_of<RegisterBase, T>()) {
            m_registers.push_back(std::dynamic_pointer_cast<RegisterBase>(ptr));
        }
        return ptr;
    }
    */

    /**
     * @brief clock
     * Simulates clocking the circuit. Registers are clocked and the propagation algorithm is run
     * @pre A call to propagate() must be done, to set the initial state of the circuit
     */
    void clock() {
        if (!m_isVerifiedAndInitialized) {
            throw std::runtime_error("Design was not verified and initialized before clocking.");
        }

        // Save register values (to correctly clock register -> register connections)
        for (auto& reg : m_registers) {
            reg->save();
        }

        // Clock registers
        for (auto& reg : m_registers) {
            reg->clock();
        }

        propagateDesign();
    }

    /**
     * @brief reset
     * Resets the circuit, setting all registers to 0 and propagates the circuit. Constants might have an affect on the
     * circuit in terms of not all component values being 0.
     */
    void reset() {
        // reset all registers
        // propagate everything combinational
        for (auto& reg : m_registers)
            reg->reset();
        propagateDesign();
    }

    void propagateDesign() {
        // Propagate circuit values - we propagate >this<, the top level component, which contains all subcomponents of
        // the design
        propagateComponent();

        // Reset propagation state of all components (uncolor graph)
        for (auto& c : m_componentGraph) {
            c.first->resetPropagation();
        }
    }

    /**
     * @brief verifyAndInitialize
     * Calls verifyDesign() to ensure that all the required inputs for each initialized object have been set, and
     * propagates the circuit to set the initial state.
     */
    void verifyAndInitialize() {
        if (m_isVerifiedAndInitialized)
            return;

        createComponentGraph();

        // Verify that all components has no undefined input signals
        for (const auto& c : m_componentGraph) {
            if (c.first->verifyInputs() == false) {
                throw std::runtime_error("A component has undefined inputs");
            }
            if (c.first->verifyOutputs() == false) {
                throw std::runtime_error("A component has undefined propagation functions");
            }
        }

        if (detectCombinationalLoop()) {
            throw std::runtime_error("Combinational loop detected in circuit");
        }

        // Propagate initial state of circuit elements through circuit (For the sake of constants being propagated)
        propagateDesign();

        m_isVerifiedAndInitialized = true;
    }

    bool cycleUtil(Component* c, std::map<Component*, bool>& visited, std::map<Component*, bool>& recurseStack) {
        visited[c] = true;
        recurseStack[c] = true;
        if (!dynamic_cast<RegisterBase*>(c))  // Graph is cut at registers
            return false;

        for (auto& neighbour : m_componentGraph[c]) {
            if (!visited[neighbour] && cycleUtil(neighbour, visited, recurseStack)) {
                return true;
            } else if (recurseStack[neighbour]) {
                return true;
            }
        }

        recurseStack[c] = false;
        return false;
    }

    bool detectCombinationalLoop() {
        std::map<Component*, bool> visited;
        for (auto& cptr : m_componentGraph) {
            visited[cptr.first] = false;
        }
        std::map<Component*, bool> recurseStack;

        for (auto& c : m_componentGraph) {
            if (visited[c.first] == false) {
                return cycleUtil(c.first, visited, recurseStack);
            } else {
                return false;
            }
        }
        return false;
    }

    const std::vector<std::unique_ptr<Component>>& getTopLevelComponents() const { return m_subcomponents; }

    const std::map<Component*, std::vector<Component*>> getDesignGraph() {
        assert(m_isVerifiedAndInitialized);
        return m_componentGraph;
    }

private:
    void createComponentGraph() {
        m_componentGraph.clear();
        getComponentGraph(m_componentGraph);

        // Gather all registers in the design
        for (auto& c : m_componentGraph) {
            if (dynamic_cast<RegisterBase*>(c.first)) {
                m_registers.insert(dynamic_cast<RegisterBase*>(c.first));
            }
        }
    }

    std::shared_ptr<Memory> m_memory;

    std::map<Component*, std::vector<Component*>> m_componentGraph;
    std::set<RegisterBase*> m_registers;
};
}  // namespace vsrtl

#endif  // VSRTL_DESIGN_H
