#ifndef RIPES_SIGNAL_H
#define RIPES_SIGNAL_H

#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <type_traits>

#include "ripes_binutils.h"
#include "ripes_defines.h"

namespace ripes {

class Component;

class SignalBase {
    friend class Component;

public:
    SignalBase() {}

    // Checks whether a propagation function has been set for the signal - required for the signal to generate its
    // next-state value
    virtual bool hasPropagationFunction() const = 0;
    virtual void propagate() = 0;

private:
};

template <uint32_t width>
class Signal : public SignalBase {
public:
    Signal() {}

    template <typename T = uint32_t>
    T value() {
        return static_cast<T>(*this);
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
    bool hasPropagationFunction() const override { return m_propagationFunction == nullptr; }
    void setPropagationFunction(std::function<std::array<bool, width>()> f) { m_propagationFunction = f; }

private:
    // Binary array representing the current value of the primitive
    std::array<bool, width> m_value;
    std::function<std::array<bool, width>()> m_propagationFunction;
};

// Connection operator
template <uint32_t width>
void operator>>(std::unique_ptr<Signal<width>>& fromThisOutput, std::unique_ptr<Signal<width>*>& toThisInput) {
    *toThisInput = &*fromThisOutput;
}

}  // namespace ripes

#endif  // RIPES_SIGNAL_H
