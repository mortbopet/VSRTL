#ifndef VSRTL_REGISTER_H
#define VSRTL_REGISTER_H

#include "../interface/vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
#include <deque>
#include <vector>

/** Registered input
 *  Constructs an inputport, outputport and register component with a given name @p input and @p width.
 * Useful for creatng shift registers or pipeline stage separating register banks.
 */
#define REGISTERED_INPUT(input, width)          \
    INPUTPORT(input##_in, width);               \
    SUBCOMPONENT(input##_reg, Register<width>); \
    OUTPUTPORT(input##_out, width)

#define CONNECT_REGISTERED_INPUT(input) \
    input##_in >> input##_reg->in;      \
    input##_reg->out >> input##_out

/** Registeren Clear/Enable input
 * Similar to above, but instantiates clear/enable registers.
 */
#define REGISTERED_CLEN_INPUT(input, width)         \
    INPUTPORT(input##_in, width);                   \
    SUBCOMPONENT(input##_reg, RegisterClEn<width>); \
    OUTPUTPORT(input##_out, width)

#define CONNECT_REGISTERED_CLEN_INPUT(input, cl, en) \
    input##_in >> input##_reg->in;                   \
    cl >> input##_reg->clear;                        \
    en >> input##_reg->enable;                       \
    input##_reg->out >> input##_out

namespace vsrtl {
namespace core {

class ClockedComponent : public Component, public SimSynchronous {
    SetGraphicsType(ClockedComponent);

public:
    ClockedComponent(std::string name, SimComponent* parent) : Component(name, parent), SimSynchronous(this) {}
    virtual void save() = 0;

    static unsigned int& reverseStackSize() {
        static unsigned int s_reverseStackSize = 100;
        return s_reverseStackSize;
    }
};

class RegisterBase : public ClockedComponent {
public:
    RegisterBase(std::string name, SimComponent* parent) : ClockedComponent(name, parent) {}

    virtual PortBase* getIn() = 0;
    virtual PortBase* getOut() = 0;
};

template <unsigned int W>
class Register : public RegisterBase {
public:
    SetGraphicsType(Register);

    Register(std::string name, SimComponent* parent) : RegisterBase(name, parent) {
        setSpecialPort("in", getIn());
        setSpecialPort("out", getOut());

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
        m_savedValue = in.template value<VSRTL_VT_U>();
    }

    void forceValue(VSRTL_VT_U /* addr */, VSRTL_VT_U value) override {
        // Sign-extension with unsigned type forces width truncation to m_width bits
        m_savedValue = signextend<VSRTL_VT_U, W>(value);
        // Forced values are a modification of the current state and thus not pushed onto the reverse stack
    }

    void reverse() override {
        if (m_reverseStack.size() > 0) {
            m_savedValue = m_reverseStack.front();
            m_reverseStack.pop_front();
        }
    }

    PortBase* getIn() override { return &in; }
    PortBase* getOut() override { return &out; }

    INPUTPORT(in, W);
    OUTPUTPORT(out, W);

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
    RegisterClEn(std::string name, SimComponent* parent) : Register<W>(name, parent) {}

    void save() override {
        this->saveToStack();
        if (enable.uValue()) {
            if (clear.uValue()) {
                this->m_savedValue = 0;
            } else {
                this->m_savedValue = this->in.template value<VSRTL_VT_U>();
            }
        }
    }

    INPUTPORT(enable, 1);
    INPUTPORT(clear, 1);
};

}  // namespace core
}  // namespace vsrtl

#endif  // VSRTL_REGISTER_H
