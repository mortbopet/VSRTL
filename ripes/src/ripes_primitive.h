#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ripes_binutils.h"
#include "ripes_defines.h"

/**
 * @brief The Primitive class
 * A primitive for all hardware components - describes the node structure in the datapath graph.
 */

namespace ripes {
class PrimitiveBase {
public:
    PrimitiveBase() {}
    virtual void propagate() = 0;
    virtual void verify() const = 0;
    virtual void resetPropagation() { m_isPropagated = false; }

    template <typename T = uint32_t>
    T getValue() {
        return static_cast<T>(*this);
    }

    // Casting operators
    virtual operator int() const = 0;
    virtual operator uint32_t() const = 0;
    virtual operator bool() const = 0;

protected:
    bool m_isPropagated = false;
};

template <uint32_t width, uint32_t inputs = 0, uint32_t additionalInputs = 0>
class Primitive : public PrimitiveBase {
public:
    static_assert(width <= REGISTERWIDTH, "Width must be in range of register width");

    Primitive(std::string displayName) {
        m_displayName = displayName;
        buildArr<width>(m_value, 0);
    }

    /**
     * @brief connect
     * Primitives can only be connected to primitives of same bit-field width. A compile-time error will be generated if
     * this is invalidly set
     * @param connectFrom
     * @param inputPort
     */
    template <uint32_t inputPort>
    void connect(std::shared_ptr<PrimitiveBase> connectFrom) {
        if (std::get<inputPort>(m_inputs) == nullptr) {
            std::get<inputPort>(m_inputs) = connectFrom;
        } else {
            throw std::runtime_error("Input port has already been assigned");
        }
    }
    template <uint32_t additionalInputPort>
    void connectAdditional(std::shared_ptr<PrimitiveBase> connectFrom) {
        std::get<additionalInputPort>(m_additionalInputs) = connectFrom;
    }

    virtual void propagate() override = 0;  // Implementation of propagate() must call propagateBase

    /**
     * @brief verify
     * Ensures that all inputs are well defined
     */
    void verify() const override {
        // Verify that all inputs have been set through connect() calls
        for (const auto& p : m_inputs) {
            ASSERT_CONNECTION_DEFINED(p)
        }
        for (const auto& p : m_additionalInputs) {
            ASSERT_CONNECTION_DEFINED(p);
        }

        // Do subtype specific verification
        verifySubtype();
    }

    /**
     * @brief verifySubtype
     * Implementation of verifySubtype() const ensures that all subtype-specific invariants are fullfilled
     */
    virtual void verifySubtype() const = 0;

    // Casting operators
    operator int() const override { return signextend<int32_t, width>(accBArr<width>(m_value)); }
    operator uint32_t() const override { return accBArr<width>(m_value); }
    operator bool() const override { return m_value[0]; }

protected:
    void propagateBase(std::function<std::array<bool, width>()> f) {
        if (!m_isPropagated) {
            propagateInputs();
            m_value = f();
            m_isPropagated = true;
        }
    }

    // Binary array representing the current value of the primitive
    std::array<bool, width> m_value{};

    std::array<std::shared_ptr<PrimitiveBase>, inputs> m_inputs;
    std::array<std::shared_ptr<PrimitiveBase>, additionalInputs>
        m_additionalInputs;  // Stuff such as multiplexer selecter, register enable etc.

private:
    void propagateInputs() {
        for (auto input : m_inputs) {
            input->propagate();
        }
        for (auto input : m_additionalInputs) {
            input->propagate();
        }
    }

    std::string m_displayName;
};
}  // namespace ripes

#endif  // PRIMITIVE_H
