#ifndef VSRTL_MANYNESTEDCOMPONENTS_H
#define VSRTL_MANYNESTEDCOMPONENTS_H

#include "vsrtl_nestedexponenter.h"

namespace vsrtl {

class DoubleNestedExponenter : public Component {
    NON_REGISTER_COMPONENT
public:
    DoubleNestedExponenter(const char* name) : Component(name) {
        in >> exp1->in;
        exp1->out >> exp2->in;

        out.setPropagationFunction(exp2->out.getFunctor());
    }
    INPUTSIGNAL(in, 32);
    OUTPUTSIGNAL(out, 32);

private:
    SUBCOMPONENT_NT(exp1, Exponenter);
    SUBCOMPONENT_NT(exp2, Exponenter);
};

class ManyNestedComponents : public Design {
public:
    static constexpr int m_cVal = 4;
    ManyNestedComponents() : Design("Many Nested Components") {
        // Connect objects
        exp1->out >> exp2->in;
        exp2->out >> exp1->in;
    }

    // Create objects
    SUBCOMPONENT_NT(exp1, DoubleNestedExponenter);
    SUBCOMPONENT_NT(exp2, DoubleNestedExponenter);
};
}  // namespace vsrtl

#endif  // VSRTL_MANYNESTEDCOMPONENTS_H
