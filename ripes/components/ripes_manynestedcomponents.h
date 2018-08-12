#ifndef RIPES_MANYNESTEDCOMPONENTS_H
#define RIPES_MANYNESTEDCOMPONENTS_H

#include "ripes_nestedexponenter.h"

namespace ripes {

class DoubleNestedExponenter : public Component {
    NON_REGISTER_COMPONENT
public:
    SUBCOMPONENT_NT(exp1, Exponenter);
    SUBCOMPONENT_NT(exp2, Exponenter);

    INPUTSIGNAL(in, 32);
    OUTPUTSIGNAL(out, 32);

    DoubleNestedExponenter() : Component("Double nested Exponenter") {
        connectSignal(in, exp1->in);
        connectSignal(exp1->out, exp2->in);

        out->setPropagationFunction(exp2->out->getFunctor());
    }
};

class ManyNestedComponents : public Architecture<3> {
public:
    static constexpr int m_cVal = 4;

    // Create objects
    SUBCOMPONENT_NT(exp1, DoubleNestedExponenter);
    SUBCOMPONENT_NT(exp2, DoubleNestedExponenter);

    ManyNestedComponents() : Architecture() {
        // Connect objects
        connectSignal(exp1->out, exp2->in);
        connectSignal(exp2->out, exp1->in);
    }
};
}  // namespace ripes

#endif  // RIPES_MANYNESTEDCOMPONENTS_H
