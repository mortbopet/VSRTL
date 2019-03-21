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
public:
    Design(std::string name) : Component(name, nullptr) {}

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
        for (const auto& reg : m_registers) {
            reg->save();
        }

        // Increment rewind-stack if possible
        if (m_rewindstackCount < Register::rewindStackSize()) {
            m_rewindstackCount++;
        }

        propagateDesign();
    }

    void rewind() {
        if (canrewind()) {
            if (!m_isVerifiedAndInitialized) {
                throw std::runtime_error("Design was not verified and initialized before rewinding.");
            }
            // Clock registers
            for (const auto& reg : m_registers) {
                reg->rewind();
            }
            m_rewindstackCount--;
            propagateDesign();
        }
    }

    /**
     * @brief reset
     * Resets the circuit, setting all registers to 0 and propagates the circuit. Constants might have an affect on the
     * circuit in terms of not all component values being 0.
     */
    void reset() {
        // reset all registers
        // propagate everything combinational
        for (const auto& reg : m_registers)
            reg->reset();
        propagateDesign();
        m_rewindstackCount = 0;
    }

    inline bool canrewind() const { return m_rewindstackCount != 0; }

    void createPropagationStack() {
        // The circuit is traversed to find the sequence of which ports may be propagated, such that all input
        // dependencies for each component are met when a port is propagated. With this, propagateDesign() may
        // sequentially ierate through the propagation stack to propagate the value of each port
        for (const auto& reg : m_registers)
            reg->propagateComponent(m_propagationStack);
    }

    void propagateDesign() {
        for (const auto& p : m_propagationStack)
            p->setPortValue();
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

        for (const auto& c : m_componentGraph) {
            // Verify that all components has no undefined input signals
            c.first->verifyComponent();
            // Initialize the component
            c.first->initialize();
        }

        if (detectCombinationalLoop()) {
            throw std::runtime_error("Combinational loop detected in circuit");
        }

        // Traverse the graph to create the optimal propagation sequence
        createPropagationStack();

        // Reset the circuit to propagate initial state
        // @todo this should be changed, such that ports initially have a value of "X" until they are assigned
        reset();

        m_isVerifiedAndInitialized = true;
    }

    bool cycleUtil(Component* c, std::map<Component*, bool>& visited, std::map<Component*, bool>& recurseStack) {
        visited[c] = true;
        recurseStack[c] = true;
        if (!dynamic_cast<Register*>(c))  // Graph is cut at registers
            return false;

        for (const auto& neighbour : m_componentGraph[c]) {
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
        for (const auto& cptr : m_componentGraph) {
            visited[cptr.first] = false;
        }
        std::map<Component*, bool> recurseStack;

        for (const auto& c : m_componentGraph) {
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

    const std::set<Register*>& getRegisters() const { return m_registers; }

private:
    void createComponentGraph() {
        m_componentGraph.clear();
        getComponentGraph(m_componentGraph);

        // Gather all registers in the design
        for (const auto& c : m_componentGraph) {
            if (dynamic_cast<Register*>(c.first)) {
                m_registers.insert(dynamic_cast<Register*>(c.first));
            }
        }
    }

    unsigned int m_rewindstackCount = 0;
    std::map<Component*, std::vector<Component*>> m_componentGraph;
    std::set<Register*> m_registers;

    std::vector<Port*> m_propagationStack;
};
}  // namespace vsrtl

#endif  // VSRTL_DESIGN_H
