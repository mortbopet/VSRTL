#ifndef VSRTL_DESIGN_H
#define VSRTL_DESIGN_H

#include "../interface/vsrtl_defines.h"
#include "vsrtl_component.h"
#include "vsrtl_memory.h"
#include "vsrtl_register.h"

#include <memory>
#include <set>
#include <type_traits>
#include <utility>

namespace vsrtl {
namespace core {

// An ADDRESSSPACE instance defines a distinct address space. Multiple memory
// components may be linked to the same address space to provide separate access
// ports to a shared address space.
#define ADDRESSSPACE(name)                                                     \
  AddressSpace *name = this->createMemory<AddressSpace>()
#define ADDRESSSPACEMM(name)                                                   \
  AddressSpaceMM *name = this->createMemory<AddressSpaceMM>()

/**
 * @brief The Design class
 * superclass for all Design descriptions
 */
class Design : public SimDesign {
public:
  Design(const std::string &name) : SimDesign(name, nullptr) {}

  /**
   * @brief clock
   * Simulates clocking the circuit. Registers are clocked and the propagation
   * algorithm is run
   * @pre A call to propagate() must be done, to set the initial state of the
   * circuit
   */
  void clock() override {
    if (!isVerifiedAndInitialized()) {
      throw std::runtime_error(
          "Design was not verified and initialized before clocking.");
    }

    // Save register values (to correctly clock register -> register
    // connections)
    for (const auto &reg : m_clockedComponents) {
      reg->save();
    }

    ClockedComponent::pushReversibleCycle();
    m_cycleCount++;
    propagateDesign();
    SimDesign::clock();
  }

  void reverse() override {
    if (canReverse()) {
      if (!isVerifiedAndInitialized()) {
        throw std::runtime_error(
            "Design was not verified and initialized before reversing.");
      }
      // Clock registers
      for (const auto &reg : m_clockedComponents) {
        reg->reverse();
      }
      ClockedComponent::popReversibleCycle();
      m_cycleCount--;
      propagateDesign();
      SimDesign::reverse();
    }
  }

  void propagate() override { propagateDesign(); }

  /**
   * @brief reset
   * Resets the circuit, setting all registers to 0 and propagates the circuit.
   * Constants might have an affect on the circuit in terms of not all component
   * values being 0.
   */
  void reset() override {
    // Reset all memories, clearing the sparse arrays and rewriting any
    // initialization data
    for (const auto &memory : m_memories) {
      memory->reset();
    }

    // reset all registers
    // propagate everything combinational
    for (const auto &reg : m_clockedComponents)
      reg->reset();
    propagateDesign();
    ClockedComponent::resetReverseStackCount();
    m_cycleCount = 0;
    SimDesign::reset();
  }

  bool canReverse() const override { return ClockedComponent::canReverse(); }
  /**
   * @brief setReverseStackSize
   * Sets the maximum number of reversible cycles to @param size and updates all
   * clocked components to reflect the new reverse stack size.
   */
  void setReverseStackSize(unsigned size) {
    ClockedComponent::setReverseStackSize(size);
    for (const auto &c : m_clockedComponents) {
      c->reverseStackSizeChanged();
    }
  }

  void createPropagationStack() {
    // The circuit is traversed to find the sequence of which ports may be
    // propagated, such that all input dependencies for each component are met
    // when a port is propagated. With this, propagateDesign() may sequentially
    // ierate through the propagation stack to propagate the value of each port
    for (const auto &reg : m_clockedComponents)
      reg->propagateComponent(m_propagationStack);
  }

  void propagateDesign() {
    for (const auto &p : m_propagationStack)
      p->setPortValue();
  }

  void setSynchronousValue(SimSynchronous *c, VSRTL_VT_U addr,
                           VSRTL_VT_U value) override {
    c->forceValue(addr, value);
    // Given the new output value of the register, the circuit must be
    // repropagated
    propagateDesign();
  }

  /**
   * @brief verifyAndInitialize
   * Calls verifyDesign() to ensure that all the required inputs for each
   * initialized object have been set, and propagates the circuit to set the
   * initial state.
   */
  void verifyAndInitialize() override {
    if (isVerifiedAndInitialized())
      return;

    createComponentGraph();

    for (const auto &c : m_componentGraph) {
      auto *comp = c.first->cast<Component>();
      if (!comp) {
        if (c.first->cast<Design>())
          continue;
        else
          assert(false && "Trying to verify unknown component");
      }
      // Verify that all components has no undefined input signals
      comp->verifyComponent();
      // Initialize the component
      comp->initialize();
    }

    if (detectCombinationalLoop()) {
      throw std::runtime_error("Combinational loop detected in circuit");
    }

    // Traverse the graph to create the optimal propagation sequence
    createPropagationStack();

    // Reset the circuit to propagate initial state
    // @todo this should be changed, such that ports initially have a value of
    // "X" until they are assigned
    reset();

    SimDesign::verifyAndInitialize();
  }

  bool cycleUtil(SimComponent *c, std::map<SimComponent *, bool> &visited,
                 std::map<SimComponent *, bool> &recurseStack) {
    visited[c] = true;
    recurseStack[c] = true;
    if (!dynamic_cast<ClockedComponent *>(c)) // Graph is cut at registers
      return false;

    for (const auto &neighbour : m_componentGraph[c]) {
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
    std::map<SimComponent *, bool> visited;
    for (const auto &cptr : m_componentGraph) {
      visited[cptr.first] = false;
    }
    std::map<SimComponent *, bool> recurseStack;

    for (const auto &c : m_componentGraph) {
      if (visited[c.first] == false) {
        return cycleUtil(c.first, visited, recurseStack);
      } else {
        return false;
      }
    }
    return false;
  }

  template <typename T>
  T *createMemory() {
    static_assert(std::is_base_of<AddressSpace, T>::value);
    auto sptr = std::make_unique<T>();
    auto *ptr = sptr.get();
    m_memories.push_back(std::move(sptr));
    return ptr;
  }

private:
  void createComponentGraph() {
    m_componentGraph.clear();
    getComponentGraph(m_componentGraph);

    // Gather all registers in the design
    for (const auto &c : m_componentGraph) {
      if (auto *cc = dynamic_cast<ClockedComponent *>(c.first)) {
        m_clockedComponents.insert(cc);
      }
      if (auto *rb = dynamic_cast<RegisterBase *>(c.first)) {
        m_registers.insert(rb);
      }
    }
  }

  std::map<SimComponent *, std::vector<SimComponent *>> m_componentGraph;
  std::set<RegisterBase *> m_registers;
  std::set<ClockedComponent *> m_clockedComponents;
  std::vector<std::unique_ptr<AddressSpace>> m_memories;

  std::vector<PortBase *> m_propagationStack;
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_DESIGN_H
