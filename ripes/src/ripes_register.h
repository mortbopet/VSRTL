#ifndef REGISTER_H
#define REGISTER_H

#include "ripes_primitive.h"

#include <algorithm>
#include <vector>

namespace ripes {

class RegisterBase {
public:
    virtual void reset() = 0;
    virtual void clock() = 0;
    virtual void save() = 0;
};

template <uint32_t width>
class Register : public RegisterBase, public Primitive<width, /*Input ports:*/ 1> {
public:
    Register() : Primitive<width, 1>("Register") {}
    void propagate() override { /* Propagation has no effect on registers*/
    }

    void verifySubtype() const override {}

protected:
    void reset() override final { buildArr<width>(this->m_value, 0); }
    void save() override final { m_savedValue = static_cast<uint32_t>(*this->m_inputs[0]); }
    void clock() override final { buildArr<width>(this->m_value, this->m_savedValue); }

    uint32_t m_savedValue;
};
}  // namespace ripes

#endif  // REGISTER_H
