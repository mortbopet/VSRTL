#ifndef REGISTER_H
#define REGISTER_H

#include "ripes_component.h"
#include "ripes_signal.h"

#include <algorithm>
#include <vector>

namespace ripes {

class RegisterBase : public Component {
public:
    virtual void reset() = 0;
    virtual void clock() = 0;
    virtual void save() = 0;
};

template <uint32_t width>
class Register : public RegisterBase {
public:
    Register() {}

    void reset() override final { m_output->setValue(buildUnsignedArr<width>(0)); }
    void save() override final { m_savedValue = (*m_input)->value<uint32_t>(); }
    void clock() override final { m_output->propagate(); }

    INPUTSIGNAL(m_input, width);
    OUTPUTSIGNAL(m_output, width);

private:
    uint32_t m_savedValue;
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
