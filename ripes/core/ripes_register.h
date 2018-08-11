#ifndef REGISTER_H
#define REGISTER_H

#include "ripes_component.h"
#include "ripes_signal.h"

#include <algorithm>
#include <vector>

namespace ripes {

class RegisterBase : public Component {
    REGISTER_COMPONENT
public:
    RegisterBase() : Component("Register Base") {}
    virtual void reset() = 0;
    virtual void clock() = 0;
    virtual void save() = 0;
};

template <uint32_t width>
class Register : public RegisterBase {
public:
    Register() {
        out->setPropagationFunction([=] { return buildUnsignedArr<width>(m_savedValue); });
        m_displayName = "Register";
    }

    void reset() override final { out->setValue(buildUnsignedArr<width>(0)); }
    void save() override final { m_savedValue = (*(*in))->template value<uint32_t>(); }
    void clock() override final { out->propagate(); }

    INPUTSIGNAL(in, width);
    OUTPUTSIGNAL(out, width);

private:
    uint32_t m_savedValue = 0;
};

}  // namespace ripes

#endif  // REGISTER_H
