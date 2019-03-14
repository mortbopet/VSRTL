#ifndef VSRTL_COMPARATOR_H
#define VSRTL_COMPARATOR_H

#include "vsrtl_component.h"

namespace vsrtl {

#define CMP_COMPONENT(classname, cmptype, op)                                                          \
    class classname : public Component {                                                               \
    public:                                                                                            \
        classname(std::string name, unsigned int width, Component* parent) : Component(name, parent) { \
            out.setWidth(1);                                                                           \
            op1.setWidth(width);                                                                       \
            op2.setWidth(width);                                                                       \
            out << [=] { return op1.template value<cmptype>() op op2.template value<cmptype>(); };     \
        }                                                                                              \
        OUTPUTPORT(out);                                                                               \
        INPUTPORT(op1);                                                                                \
        INPUTPORT(op2);                                                                                \
    };

CMP_COMPONENT(Sge, VSRTL_VT_S, >=)
CMP_COMPONENT(Slt, VSRTL_VT_S, <)
CMP_COMPONENT(Uge, VSRTL_VT_U, >=)
CMP_COMPONENT(Ult, VSRTL_VT_U, <)
CMP_COMPONENT(Eq, VSRTL_VT_U, ==)

}  // namespace vsrtl

#endif  // VSRTL_COMPARATOR_H
