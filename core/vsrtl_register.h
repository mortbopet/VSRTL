#ifndef REGISTER_H
#define REGISTER_H

#include "vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
#include <deque>
#include <vector>

namespace vsrtl {

DefineGraphicsType(ClockedComponent);
class ClockedComponent : public Component {
    SetGraphicsType(ClockedComponent);

public:
    ClockedComponent(std::string name, Component* parent) : Component(name, parent) {}
    virtual void reset() = 0;
    virtual void save() = 0;
    virtual void rewind() = 0;

    bool isClockedComponent() const override { return true; }

    static unsigned int& rewindStackSize() {
        static unsigned int s_rewindstackSize = 100;
        return s_rewindstackSize;
    }
};

class RegisterBase : public ClockedComponent {
public:
    RegisterBase(std::string name, Component* parent) : ClockedComponent(name, parent) {}

    virtual void forceValue(VSRTL_VT_U value) = 0;
    virtual PortBase* getIn() = 0;
    virtual PortBase* getOut() = 0;
};

DefineGraphicsType(Register);
template <unsigned int W>
class Register : public RegisterBase {
public:
    SetGraphicsType(Register);

    Register(std::string name, Component* parent) : RegisterBase(name, parent) {
        // Calling out.propagate() will clock the register the register
        out << ([=] { return m_savedValue; });
    }

    void reset() {
        m_savedValue = 0;
        m_rewindstack.clear();
    }
    void save() {
        saveToStack();
        m_savedValue = in.template value<VSRTL_VT_U>();
    }

    void forceValue(VSRTL_VT_U value) {
        // Sign-extension with unsigned type forces width truncation to m_width bits
        m_savedValue = signextend<VSRTL_VT_U, W>(value);
        // Forced values are a modification of the current state and thus not pushed onto the rewind stack
    }

    void rewind() {
        if (m_rewindstack.size() > 0) {
            m_savedValue = m_rewindstack.front();
            m_rewindstack.pop_front();
        }
    }

    PortBase* getIn() override { return &in; }
    PortBase* getOut() override { return &out; }

    INPUTPORT(in, W);
    OUTPUTPORT(out, W);

private:
    void saveToStack() {
        m_rewindstack.push_front(m_savedValue);
        if (m_rewindstack.size() > rewindStackSize()) {
            m_rewindstack.pop_back();
        }
    }

    VSRTL_VT_U m_savedValue = 0;

    std::deque<VSRTL_VT_U> m_rewindstack;
};

}  // namespace vsrtl

#endif  // REGISTER_H
