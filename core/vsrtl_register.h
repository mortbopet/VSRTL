#ifndef REGISTER_H
#define REGISTER_H

#include "vsrtl_binutils.h"
#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
#include <deque>
#include <vector>

namespace vsrtl {

class Register : public Component {
public:
    std::type_index getTypeId() const override { return std::type_index(typeid(Register)); }
    Register(std::string name, unsigned int width, Component* parent) : m_width(width), Component(name, parent) {
        // Calling out.propagate() will clock the register the register
        out << ([=] { return m_savedValue; });

        in.setWidth(width);
        out.setWidth(width);
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
        m_savedValue = signextend<VSRTL_VT_U>(value, m_width);
        // Forced values are a modification of the current state and thus not pushed onto the rewind stack
    }

    void rewind() {
        if (m_rewindstack.size() > 0) {
            m_savedValue = m_rewindstack.front();
            m_rewindstack.pop_front();
        }
    }

    INPUTPORT(in);
    OUTPUTPORT(out);

    bool isRegister() const override { return true; }

    static unsigned int& rewindStackSize() {
        static unsigned int s_rewindstackSize = 100;
        return s_rewindstackSize;
    }

private:
    void saveToStack() {
        m_rewindstack.push_front(m_savedValue);
        if (m_rewindstack.size() > rewindStackSize()) {
            m_rewindstack.pop_back();
        }
    }

    VSRTL_VT_U m_savedValue = 0;

    std::deque<VSRTL_VT_U> m_rewindstack;

    unsigned int m_width;
};

}  // namespace vsrtl

#endif  // REGISTER_H
