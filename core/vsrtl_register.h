#ifndef REGISTER_H
#define REGISTER_H

#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
#include <deque>
#include <vector>

namespace vsrtl {

class Register : public Component {
public:
    std::type_index getTypeId() const override { return std::type_index(typeid(Register)); }
    Register(std::string name, unsigned int width) : m_width(width), Component(name) {
        out << ([=] { return m_savedValue; });

        in.setWidth(width);
        out.setWidth(width);
    }

    void reset() {
        m_savedValue = 0;
        out.propagate();
        m_rewindstack.clear();
    }
    void save() {
        m_rewindstack.push_front(m_savedValue);
        if (m_rewindstack.size() > rewindStackSize()) {
            m_rewindstack.pop_back();
        }
        m_savedValue = in.template value<VSRTL_VT_U>();
    }
    void clock() { out.propagate(); }
    void rewind() {
        if (m_rewindstack.size() > 0) {
            m_savedValue = m_rewindstack.front();
            m_rewindstack.pop_front();
            out.propagate();
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
    VSRTL_VT_U m_savedValue = 0;

    std::deque<VSRTL_VT_U> m_rewindstack;

    unsigned int m_width;
};

}  // namespace vsrtl

#endif  // REGISTER_H
