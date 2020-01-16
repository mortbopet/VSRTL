#ifndef REGISTER_H
#define REGISTER_H

#include "../interface/vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
#include <deque>
#include <vector>

namespace vsrtl {
namespace core {

class ClockedComponent : public Component, public SimSynchronous {
    SetGraphicsType(ClockedComponent);

public:
    ClockedComponent(std::string name, SimComponent* parent) : Component(name, parent), SimSynchronous(this) {}
    virtual void save() = 0;

    static unsigned int& rewindStackSize() {
        static unsigned int s_rewindstackSize = 100;
        return s_rewindstackSize;
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
        m_rewindstack.clear();
    }

    void save() override {
        saveToStack();
        m_savedValue = in.template value<VSRTL_VT_U>();
    }

    void forceValue(VSRTL_VT_U /* addr */, VSRTL_VT_U value) override {
        // Sign-extension with unsigned type forces width truncation to m_width bits
        m_savedValue = signextend<VSRTL_VT_U, W>(value);
        // Forced values are a modification of the current state and thus not pushed onto the rewind stack
    }

    void rewind() override {
        if (m_rewindstack.size() > 0) {
            m_savedValue = m_rewindstack.front();
            m_rewindstack.pop_front();
        }
    }

    PortBase* getIn() override { return &in; }
    PortBase* getOut() override { return &out; }

    INPUTPORT(in, W);
    OUTPUTPORT(out, W);

protected:
    void saveToStack() {
        m_rewindstack.push_front(m_savedValue);
        if (m_rewindstack.size() > rewindStackSize()) {
            m_rewindstack.pop_back();
        }
    }

    VSRTL_VT_U m_savedValue = 0;
    VSRTL_VT_U m_initvalue = 0;
    std::deque<VSRTL_VT_U> m_rewindstack;
};

template <unsigned int W>
class RegisterEn : public Register<W> {
public:
    RegisterEn(std::string name, SimComponent* parent) : Register<W>(name, parent) {}

    void save() override {
        this->saveToStack();
        if (enable.uValue()) {
            this->m_savedValue = this->in.template value<VSRTL_VT_U>();
        }
    }

    INPUTPORT(enable, 1);
};

class RegisterBank : public Component {
public:
    SetGraphicsType(RegisterBank);
    RegisterBank(std::string name, SimComponent* parent) : Component(name, parent) {}

    INPUTPORT(clear, 1);
    INPUTPORT(enable, 1);
};

}  // namespace core
}  // namespace vsrtl

#endif  // REGISTER_H
