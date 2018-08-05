#ifndef REGISTER_H
#define REGISTER_H

#include "ripes_component.h"
#include "ripes_signal.h"

#include <algorithm>
#include <vector>

namespace ripes {

class RegisterBase : public Component {
public:
    RegisterBase() : Component("Register Base") {}
    virtual void reset() = 0;
    virtual void clock() = 0;
    virtual void save() = 0;
    void checkIfRegister() override {
        // Stops the propagation graph traversal since a register has been encountered
        m_isPropagated = true;
    }
};

template <uint32_t width>
class Register : public RegisterBase {
public:
    Register() {
        m_output->setPropagationFunction([=] { return buildUnsignedArr<width>(m_savedValue); });
    }

    void reset() override final { m_output->setValue(buildUnsignedArr<width>(0)); }
    void save() override final { m_savedValue = (*m_input)->value<uint32_t>(); }
    void clock() override final { m_output->propagate(); }

    INPUTSIGNAL(m_input, width);
    OUTPUTSIGNAL(m_output, width);

private:
    uint32_t m_savedValue = 0;
};

// Connection operators
template <uint32_t width>
inline void operator>>(std::unique_ptr<Register<width>>& r, std::unique_ptr<Signal<width>*>& toInput) {
    *toInput = r->m_output.get();
}

template <uint32_t width>
inline void operator>>(std::unique_ptr<Signal<width>>& s, std::unique_ptr<Register<width>>& r) {
    *r->m_input = s.get();
}

}  // namespace ripes

#endif  // REGISTER_H
