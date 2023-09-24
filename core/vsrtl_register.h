#ifndef VSRTL_REGISTER_H
#define VSRTL_REGISTER_H

#include "../interface/vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
#include <deque>
#include <vector>

/** Registered input
 *  Constructs an inputport, outputport and register component with a given name
 * @p input and @p width. Useful for creatng shift registers or pipeline stage
 * separating register banks.
 */
#define REGISTERED_INPUT(input, width)                                         \
  INPUTPORT(input##_in, width);                                                \
  SUBCOMPONENT(input##_reg, Register<width>);                                  \
  OUTPUTPORT(input##_out, width)

#define CONNECT_REGISTERED_INPUT(input)                                        \
  input##_in >> input##_reg->in;                                               \
  input##_reg->out >> input##_out

/** Registeren Clear/Enable input
 * Similar to above, but instantiates clear/enable registers.
 */
#define REGISTERED_CLEN_INPUT(input, width)                                    \
  INPUTPORT(input##_in, width);                                                \
  SUBCOMPONENT(input##_reg, RegisterClEn<width>);                              \
  OUTPUTPORT(input##_out, width)

#define CONNECT_REGISTERED_CLEN_INPUT(input, cl, en)                           \
  input##_in >> input##_reg->in;                                               \
  cl >> input##_reg->clear;                                                    \
  en >> input##_reg->enable;                                                   \
  input##_reg->out >> input##_out

namespace vsrtl {
namespace core {

class ClockedComponent : public Component, public SimSynchronous {
  SetGraphicsType(ClockedComponent);

public:
  ClockedComponent(const std::string &name, SimComponent *parent)
      : Component(name, parent), SimSynchronous(this) {}
  virtual void save() = 0;

  /**
   * @brief Reverse stack management
   * The following functions manages a static count of the current number of
   * reversible cycles in the design. It is expected that modifications to the
   * reverse stack counters are performed solely by the top-level managing
   * design.
   */
  static void setReverseStackSize(unsigned size) {
    getReverseStack().max = size;

    if (getReverseStack().current > size) {
      getReverseStack().current = size;
    }
  }
  static unsigned reverseStackSize() { return getReverseStack().max; }
  static unsigned reversibleCycles() { return getReverseStack().current; }
  static void resetReverseStackCount() { getReverseStack().current = 0; }
  static void pushReversibleCycle() {
    // Increment reverse-stack count if possible
    if (reversibleCycles() < reverseStackSize()) {
      getReverseStack().current++;
    }
  }
  static void popReversibleCycle() {
    if (!canReverse()) {
      throw std::runtime_error(
          "Tried to reverse the design with empty reverse stacks");
    }
    getReverseStack().current--;
  }
  static bool canReverse() { return getReverseStack().current != 0; }

  /**
   * @brief reverseStackSizeChanged
   * Whenever the reverse stack changes, all synchronous elements may check
   * whether they need to delete cycles within their current reverse stack.
   */
  virtual void reverseStackSizeChanged() = 0;

private:
  struct ReverseStackCounter {
    unsigned max =
        100; // Maximum number of cycles on clocked components reverse stacks
    unsigned current = 0; // Current number of reversible cycles
  };
  static ReverseStackCounter &getReverseStack() {
    static ReverseStackCounter reverseStackCounter;
    return reverseStackCounter;
  }
};

class RegisterBase : public ClockedComponent {
public:
  RegisterBase(const std::string &name, SimComponent *parent)
      : ClockedComponent(name, parent) {}

  virtual PortBase *getIn() = 0;
  virtual PortBase *getOut() = 0;
};

template <unsigned int W>
class Register : public RegisterBase {
public:
  SetGraphicsType(Register);

  Register(const std::string &name, SimComponent *parent)
      : RegisterBase(name, parent) {
    // Calling out.propagate() will clock the register the register
    out << ([=] { return m_savedValue; });
  }

  void setInitValue(VSRTL_VT_U value) { m_initvalue = value; }

  void reset() override {
    m_savedValue = m_initvalue;
    m_reverseStack.clear();
  }

  void save() override {
    saveToStack();
    m_savedValue = in.uValue();
  }

  void forceValue(VSRTL_VT_U /* addr */, VSRTL_VT_U value) override {
    // Sign-extension with unsigned type forces width truncation to m_width bits
    m_savedValue = signextend<W>(value);
    // Forced values are a modification of the current state and thus not pushed
    // onto the reverse stack
  }

  void reverse() override {
    if (m_reverseStack.size() > 0) {
      m_savedValue = m_reverseStack.front();
      m_reverseStack.pop_front();
    }
  }

  PortBase *getIn() override { return &in; }
  PortBase *getOut() override { return &out; }

  INPUTPORT(in, W);
  OUTPUTPORT(out, W);

  void reverseStackSizeChanged() override {
    if (reverseStackSize() < m_reverseStack.size()) {
      m_reverseStack.resize(m_reverseStack.size());
    }
  }

protected:
  void saveToStack() {
    m_reverseStack.push_front(m_savedValue);
    if (m_reverseStack.size() > reverseStackSize()) {
      m_reverseStack.pop_back();
    }
  }

  VSRTL_VT_U m_savedValue = 0;
  VSRTL_VT_U m_initvalue = 0;
  std::deque<VSRTL_VT_U> m_reverseStack;
};

// Synchronous clear/enable register
template <unsigned int W>
class RegisterClEn : public Register<W> {
public:
  RegisterClEn(const std::string &name, SimComponent *parent)
      : Register<W>(name, parent) {}

  void save() override {
    this->saveToStack();
    if (enable.uValue()) {
      if (clear.uValue()) {
        this->m_savedValue = 0;
      } else {
        this->m_savedValue = this->in.uValue();
      }
    }
  }

  INPUTPORT(enable, 1);
  INPUTPORT(clear, 1);
};

template <unsigned int W>
class ShiftRegister : public RegisterBase {
public:
  SetGraphicsType(Register);

  ShiftRegister(const std::string &name, SimComponent *parent)
      : RegisterBase(name, parent) {
    stages.setTooltip("Number of shift register stages");
    stages.setOptions({1, 100});
    stages.changed.Connect(this, &ShiftRegister::stagesChanged);
    stagesChanged(); // Default initialize

    // Calling out.propagate() will clock the register the register.
    // Output the value for the last register in the shift register array
    out << ([=] { return m_savedValues.at(stages.getValue() - 1); });
  }

  void setInitValue(VSRTL_VT_U value) { m_initvalue = value; }

  void reset() override {
    for (unsigned i = 0; i < m_savedValues.size(); i++) {
      m_savedValues[i] = m_initvalue;
    }
    m_reverseStack.clear();
  }

  void save() override {
    m_reverseStack.push_front(m_savedValues.at(stages.getValue() - 1));
    if (m_reverseStack.size() > reverseStackSize()) {
      m_reverseStack.pop_back();
    }
    // Rotate to the right and store new value as first register
    std::rotate(m_savedValues.rbegin(), m_savedValues.rbegin() + 1,
                m_savedValues.rend());
    m_savedValues.at(0) = in.uValue();
  }

  void forceValue(VSRTL_VT_U /* addr */, VSRTL_VT_U value) override {
    // Sign-extension with unsigned type forces width truncation to m_width bits
    m_savedValues[0] = signextend<VSRTL_VT_U, W>(value);
    // Forced values are a modification of the current state and thus not pushed
    // onto the reverse stack
  }

  void reverse() override {
    if (m_reverseStack.size() > 0) {
      // Rotate to the left and store popped value as last register
      std::rotate(m_savedValues.begin(), m_savedValues.begin() + 1,
                  m_savedValues.end());
      m_savedValues.at(stages.getValue() - 1) = m_reverseStack.front();
      m_reverseStack.pop_front();
    }
  }

  PortBase *getIn() override { return &in; }
  PortBase *getOut() override { return &out; }

  INPUTPORT(in, W);
  OUTPUTPORT(out, W);
  PARAMETER(stages, int, 2);

  void reverseStackSizeChanged() override {
    if (reverseStackSize() < m_reverseStack.size()) {
      m_reverseStack.resize(m_reverseStack.size());
    }
  }

protected:
  void stagesChanged() { m_savedValues.resize(stages.getValue()); }

  std::vector<VSRTL_VT_U> m_savedValues;
  VSRTL_VT_U m_initvalue = 0;
  std::deque<VSRTL_VT_U> m_reverseStack;
};

} // namespace core
} // namespace vsrtl

#endif // VSRTL_REGISTER_H
