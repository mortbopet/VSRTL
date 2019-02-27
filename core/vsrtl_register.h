#ifndef REGISTER_H
#define REGISTER_H

#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
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
    }
    void save() { m_savedValue = in.template value<uint32_t>(); }
    void clock() { out.propagate(); }

    INPUTPORT(in);
    OUTPUTPORT(out);

    bool isRegister() const override { return true; }

private:
    VSRTL_VT_U m_savedValue = 0;

    unsigned int m_width;
};

}  // namespace vsrtl

#endif  // REGISTER_H
