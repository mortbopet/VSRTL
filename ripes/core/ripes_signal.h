#ifndef RIPES_SIGNAL_H
#define RIPES_SIGNAL_H

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>

#include "ripes_binutils.h"
#include "ripes_defines.h"

// Signals cannot exist outside of components!

namespace ripes {

class Component;

class SignalBase {
    friend class Component;

public:
    SignalBase(Component* parent) : m_parent(parent) {}

    // Checks whether a propagation function has been set for the signal - required for the signal to generate its
    // next-state value
    virtual bool hasPropagationFunction() const = 0;
    virtual void propagate() = 0;
    Component* getParent() { return m_parent; }

private:
    Component* m_parent = nullptr;
};

template <uint32_t width>
class Signal : public SignalBase {
public:
    Signal(Component* parent) : SignalBase(parent) {}

    template <typename T = uint32_t>
    T value() {
        return static_cast<T>(*this);
    }

    std::function<std::array<bool, width>()> getFunctor() {
        return [=] { return m_value; };
    }

    // Casting operators
    operator int() const { return signextend<int32_t, width>(accBVec<width>(m_value)); }
    operator uint32_t() const { return accBVec<width>(m_value); }
    operator bool() const { return m_value[0]; }

    /**
     * @brief setValue
     * Used when hard-setting a signals value (ie. used by registers when resetting their output signals
     * @param v
     */
    void setValue(std::array<bool, width> v) { m_value = v; }

    void propagate() { m_value = m_propagationFunction(); }
    bool hasPropagationFunction() const override { return m_propagationFunction != nullptr; }
    void setPropagationFunction(std::function<std::array<bool, width>()> f) { m_propagationFunction = f; }

private:
    // Binary array representing the current value of the primitive
    std::array<bool, width> m_value = buildUnsignedArr<width>(0);
    std::function<std::array<bool, width>()> m_propagationFunction;
};

template <uint32_t width>
void connectSignal(Signal<width>*& fromThisOutput, Signal<width>***& toThisInput) {
    *(*toThisInput) = fromThisOutput;
}

template <uint32_t width>
void connectSignal(Signal<width>***& fromThisInput, Signal<width>***& toThisInput) {
    *toThisInput = *fromThisInput;
}

}  // namespace ripes

#endif  // RIPES_SIGNAL_H
