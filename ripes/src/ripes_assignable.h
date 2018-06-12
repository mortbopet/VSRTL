#ifndef ASSIGNABLE_H
#define ASSIGNABLE_H

#include <functional>
#include "ripes_binutils.h"
#include "ripes_primitive.h"

namespace ripes {

template <uint32_t width>
class Assignable : public Primitive<width> {
public:
    Assignable() : Primitive("Assignable") {}

    void propagate() override { this->propagateBase(m_f); }
    void verifySubtype() const override { ASSERT_CONNECTION_EXPR(m_f != nullptr); }

    void setFunctor(std::function<std::array<bool, width>()> f) { m_f = f; }

private:
    std::function<std::array<bool, width>()> m_f;
};
}  // namespace ripes

#endif  // ASSIGNABLE_H
