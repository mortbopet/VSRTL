#ifndef REGISTER_H
#define REGISTER_H

#include "vsrtl_component.h"
#include "vsrtl_port.h"

#include <algorithm>
#include <vector>

namespace vsrtl {

class RegisterBase : public Component {
    REGISTER_COMPONENT
public:
    RegisterBase(const char* name) : Component(name) {}
    virtual void reset() = 0;
    virtual void clock() = 0;
    virtual void save() = 0;
};

template <unsigned int width>
class Register : public RegisterBase {
public:
    const char* getBaseType() const override { return "Register"; }
    Register(const char* name) : RegisterBase(name) {
        out << ([=] { return m_savedValue; });
    }

    void reset() override final {
        m_savedValue = 0;
        out.propagate();
    }
    void save() override final { m_savedValue = in.template value<uint32_t>(); }
    void clock() override final { out.propagate(); }

    INPUTPORT(in, width);
    OUTPUTPORT(out, width);

private:
    VSRTL_VT_U m_savedValue = 0;
};

}  // namespace vsrtl

#endif  // REGISTER_H
