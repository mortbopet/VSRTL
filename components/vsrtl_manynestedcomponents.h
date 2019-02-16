#ifndef VSRTL_MANYNESTEDCOMPONENTS_H
#define VSRTL_MANYNESTEDCOMPONENTS_H

#include "vsrtl_nestedexponenter.h"

namespace vsrtl {

class DoubleNestedExponenter : public Component {
public:
    DoubleNestedExponenter(std::string name) : Component(name) {
        in.setWidth(32);
        out.setWidth(32);

        in >> exp1->expIn;
        exp1->out >> exp2->expIn;

        exp2->out >> out;
    }
    INPUTPORT(in);
    OUTPUTPORT(out);

private:
    SUBCOMPONENT(exp1, Exponenter);
    SUBCOMPONENT(exp2, Exponenter);
};

class ManyNestedComponents : public Design {
public:
    static constexpr int m_cVal = 4;
    ManyNestedComponents() : Design("Many nested components") {
        // Connect objects
        exp1->out >> exp2->in;
        exp2->out >> exp1->in;
    }

    // Create objects
    SUBCOMPONENT(exp1, DoubleNestedExponenter);
    SUBCOMPONENT(exp2, DoubleNestedExponenter);
};
}  // namespace vsrtl

#endif  // VSRTL_MANYNESTEDCOMPONENTS_H
