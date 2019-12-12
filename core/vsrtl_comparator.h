#ifndef VSRTL_COMPARATOR_H
#define VSRTL_COMPARATOR_H

#include "vsrtl_component.h"

namespace vsrtl {
namespace core {

#define CMP_COMPONENT(classname, cmptype, op)                                                      \
    template <unsigned int W>                                                                      \
    class classname : public Component {                                                           \
    public:                                                                                        \
        classname(std::string name, SimComponent* parent) : Component(name, parent) {              \
            out << [=] { return op1.template value<cmptype>() op op2.template value<cmptype>(); }; \
        }                                                                                          \
        OUTPUTPORT(out, 1);                                                                        \
        INPUTPORT(op1, W);                                                                         \
        INPUTPORT(op2, W);                                                                         \
    };

CMP_COMPONENT(Sge, VSRTL_VT_S, >=)
CMP_COMPONENT(Slt, VSRTL_VT_S, <)
CMP_COMPONENT(Uge, VSRTL_VT_U, >=)
CMP_COMPONENT(Ult, VSRTL_VT_U, <)
CMP_COMPONENT(Eq, VSRTL_VT_U, ==)

}  // namespace core
}  // namespace vsrtl

#endif  // VSRTL_COMPARATOR_H
