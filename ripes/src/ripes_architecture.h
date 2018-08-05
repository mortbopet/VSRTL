#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include "ripes_component.h"
#include "ripes_defines.h"
#include "ripes_memory.h"
#include "ripes_register.h"

#include <memory>
#include <type_traits>
#include <utility>

namespace ripes {

/**
 * @brief The Architecture class
 * superclass for all architecture descriptions
 */

template <int stageCount>
class Architecture : public Component {
public:
    static_assert(stageCount >= 0, "number of stages must be positive");

    Architecture(int flags = 0) {
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

        if (specialRegisterFile) {
            // Enable writing and reading from register file in the same cycle
            /** @todo clock and propagate register file before rest of circuit*/
        }

        // Each registers input value is saved before clocking it, to ensure that register -> register connections
        // are clocked properly
        std::for_each(m_registers.begin(), m_registers.end(), [](auto& reg) { reg->save(); });
        std::for_each(m_registers.begin(), m_registers.end(), [](auto& reg) { reg->clock(); });

        // Propagate circuit values
        propagateComponent();
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

    /**
     * @brief verifyAndInitialize
     * Calls verifyDesign() to ensure that all the required inputs for each initialized object have been set, and
     * propagates the circuit to set the initial state.
     */
    void verifyAndInitialize() {
        // verifyDesign();

        // Propagate initial state of circuit elements through circuit (For the sake of constants being propagated)
        propagateComponent();
    }

    void loadProgram(const std::vector<char>& p) {
        if (m_memory == nullptr) {
            throw std::runtime_error("The architecture does not contain a memory. Set");
        }
    };

private:
    std::array<std::vector<RegisterBase*>, stageCount> m_stageRegisterBanks;

    std::vector<std::shared_ptr<RegisterBase>> m_registers;

    std::unique_ptr<Memory> m_memory;

    /*
    std::shared_ptr<Assignable<REGISTERWIDTH>> m_instructionutionMemory;
    std::shared_ptr<Assignable<REGISTERWIDTH>> m_dataMemory;
*/
};
}  // namespace ripes

#endif  // ARCHITECTURE_H
