#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

#include "ripes_defines.h"
#include "ripes_memory.h"
#include "ripes_register.h"
#include "ripes_registerfile.h"

#include <memory>
#include <type_traits>
#include <utility>

namespace ripes {

/**
 * @brief The Architecture class
 * superclass for all architecture descriptions
 */

template <int stageCount>
class Architecture {
public:
    static_assert(stageCount >= 0, "number of stages must be positive");

    Architecture(int flags = 0) {
        // Primitives are now recorded, and architecture-constant objects can be instantiated
        if (flags & dataMemory)
            m_dataMemory = create<Assignable<REGISTERWIDTH>>();
        if (flags & instructionMemory)
            m_instrutionMemory = create<Assignable<REGISTERWIDTH>>();
    }

    template <typename T, typename... Args>
    std::shared_ptr<T> create(Args&&... args) {
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        this->m_primitives.push_back(ptr);
        if (std::is_base_of<RegisterBase, T>()) {
            m_registers.push_back(std::dynamic_pointer_cast<RegisterBase>(ptr));
        }
        return ptr;
    }

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
        propagate();
    }

    /**
     * @brief reset
     * Resets the circuit, setting all registers to 0 and propagates the circuit. Constants might have an affect on the
     * circuit in terms of not all primitive values being 0.
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
        verifyDesign();

        // Propagate initial state of circuit elements through circuit (For the sake of constants being propagated)
        propagate();
    }

    void loadProgram(const std::vector<char>& p) {
        if (m_memory == nullptr) {
            throw std::runtime_error("The architecture does not contain a memory. Set");
        }
    };

private:
    /**
     * @brief propagate
     */
    void propagate() {
        /** @todo explain the propagation algorithm*/
        for (auto primitive : m_primitives) {
            primitive->propagate();
        }

        // Reset propagation flag of all components
        for (auto primitive : m_primitives) {
            primitive->resetPropagation();
        }
    }

    /**
     * @brief verifyDesign
     * Calls verify() on each object in the circuit and checks for combinational loops
     */
    void verifyDesign() { /** @todo check for combinational loops */
        for (const auto& primitive : m_primitives) {
            primitive->verify();
        }
        isVerifiedAndInitialized = true;
    }
    bool isVerifiedAndInitialized = false;

    std::array<std::vector<RegisterBase*>, stageCount> m_stageRegisterBanks;

    std::vector<std::shared_ptr<PrimitiveBase>> m_primitives;
    std::vector<std::shared_ptr<RegisterBase>> m_registers;

    std::unique_ptr<Memory> m_memory;
    std::shared_ptr<Assignable<REGISTERWIDTH>> m_instrutionMemory;
    std::shared_ptr<Assignable<REGISTERWIDTH>> m_dataMemory;
};
}  // namespace ripes

#endif  // ARCHITECTURE_H
