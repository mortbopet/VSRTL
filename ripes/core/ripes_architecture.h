#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include "ripes_component.h"
#include "ripes_defines.h"
#include "ripes_memory.h"
#include "ripes_register.h"

#include <memory>
#include <set>
#include <type_traits>
#include <utility>

namespace ripes {

/**
 * @brief The Architecture class
 * superclass for all architecture descriptions
 */

class Architecture : public Component {
    NON_REGISTER_COMPONENT
public:
    Architecture(int flags = 0) : Component("Top") {
        // components are now recorded, and architecture-constant objects can be instantiated
        if (flags & dataMemory)
            OUTPUTSIGNAL(m_dataMemory, REGISTERWIDTH);
        if (flags & instructionMemory)
            OUTPUTSIGNAL(m_instructionMemory, REGISTERWIDTH);
    }

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
     * @param specialRegisterFile @todo document this
     */
    void clock(bool specialRegisterFile = false) {
        if (!isVerifiedAndInitialized) {
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
        std::for_each(m_registers.begin(), m_registers.end(), [](auto& reg) { reg->reset(); });
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

        isVerifiedAndInitialized = true;
    }

    bool cycleUtil(Component* c, std::map<Component*, bool>& visited, std::map<Component*, bool>& recurseStack) {
        visited[c] = true;
        recurseStack[c] = true;
        if (!dynamic_cast<RegisterBase*>(c)) {  // Graph is cut at registers
            return false;

            for (auto& neighbour : m_componentGraph[c]) {
                if (!visited[neighbour] && cycleUtil(neighbour, visited, recurseStack)) {
                    return true;
                } else if (recurseStack[neighbour]) {
                    return true;
                }
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

    void loadProgram(const std::vector<char>& p) {
        if (m_memory == nullptr) {
            throw std::runtime_error("The architecture does not contain a memory. Set");
        }
    }

    const std::vector<Component*>& getTopLevelComponents() const { return COMPONENT_CONTAINER; }

    const std::map<Component*, std::vector<Component*>> getDesignGraph() {
        assert(isVerifiedAndInitialized);
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

    /*
    std::shared_ptr<Assignable<REGISTERWIDTH>> m_instructionutionMemory;
    std::shared_ptr<Assignable<REGISTERWIDTH>> m_dataMemory;
*/
};
}  // namespace ripes

#endif  // ARCHITECTURE_H
