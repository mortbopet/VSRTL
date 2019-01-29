#ifndef VSRTL_SIGNAL_H
#define VSRTL_SIGNAL_H

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>

#include "vsrtl_binutils.h"
#include "vsrtl_defines.h"

// Signals cannot exist outside of components!

namespace vsrtl {

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
    virtual unsigned int width() const = 0;

    // Value access operators
    virtual explicit operator int() const = 0;
    virtual explicit operator unsigned int() const = 0;
    virtual explicit operator bool() const = 0;

private:
    Component* m_parent = nullptr;
};

template <unsigned int bitwidth>
class Signal : public SignalBase {
public:
    Signal(Component* parent) : SignalBase(parent) {}

    unsigned int width() const override { return bitwidth; }

    template <typename T = unsigned int>
    T value() {
        return static_cast<T>(*this);
    }

    std::function<std::array<bool, bitwidth>()> getFunctor() {
        return [=] { return m_value; };
    }

    // Casting operators
    explicit operator int() const override { return signextend<int32_t, bitwidth>(accBVec<bitwidth>(m_value)); }
    explicit operator unsigned int() const override { return accBVec<bitwidth>(m_value); }
    explicit operator bool() const override { return m_value[0]; }

    /**
     * @brief setValue
     * Used when hard-setting a signals value (ie. used by registers when resetting their output signals
     * @param v
     */
    void setValue(std::array<bool, bitwidth> v) { m_value = v; }

    void propagate() override { m_value = m_propagationFunction(); }
    bool hasPropagationFunction() const override { return m_propagationFunction != nullptr; }
    void setPropagationFunction(std::function<std::array<bool, bitwidth>()> f) { m_propagationFunction = f; }

private:
    // Binary array representing the current value of the primitive
    std::array<bool, bitwidth> m_value = buildUnsignedArr<bitwidth>(0);
    std::function<std::array<bool, bitwidth>()> m_propagationFunction;
};

template <unsigned int bitwidth>
void connectSignal(Signal<bitwidth>*& fromThisOutput, Signal<bitwidth>***& toThisInput) {
    *(*toThisInput) = fromThisOutput;
}

template <unsigned int bitwidth>
void connectSignal(Signal<bitwidth>***& fromThisInput, Signal<bitwidth>***& toThisInput) {
    *toThisInput = *fromThisInput;
}

}  // namespace vsrtl

#endif  // VSRTL_SIGNAL_H
