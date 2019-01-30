#ifndef REGISTER_H
#define REGISTER_H

#include "vsrtl_component.h"
#include "vsrtl_signal.h"

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
    Register(const char* name) : RegisterBase(name) {
        out.setPropagationFunction([=] { return buildUnsignedArr<width>(m_savedValue); });
    }

    void reset() override final {
        out.setValue(buildUnsignedArr<width>(0));
        m_savedValue = 0;
    }
    void save() override final { m_savedValue = in.template value<uint32_t>(); }
    void clock() override final { out.propagate(); }

    INPUTSIGNAL(in, width);
    OUTPUTSIGNAL(out, width);

private:
    uint32_t m_savedValue = 0;
};

}  // namespace vsrtl

#endif  // REGISTER_H
