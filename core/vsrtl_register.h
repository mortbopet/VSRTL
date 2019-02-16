#ifndef REGISTER_H
#define REGISTER_H

#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
#include <vector>

namespace vsrtl {

class RegisterBase : public Component {
public:
    RegisterBase(std::string name) : Component(name) {}
    virtual void reset() = 0;
    virtual void clock() = 0;
    virtual void save() = 0;

    bool isRegister() const override { return true; }
};

class Register : public RegisterBase {
public:
    const char* getBaseType() const override { return "Register"; }
    Register(std::string name, unsigned int width) : RegisterBase(name), m_width(width) {
        out << ([=] { return m_savedValue; });

        in.setWidth(width);
        out.setWidth(width);
    }

    void reset() override final {
        m_savedValue = 0;
        out.propagate();
    }
    void save() override final { m_savedValue = in.template value<uint32_t>(); }
    void clock() override final { out.propagate(); }

    INPUTPORT(in);
    OUTPUTPORT(out);

private:
    VSRTL_VT_U m_savedValue = 0;

    unsigned int m_width;
};

}  // namespace vsrtl

#endif  // REGISTER_H
