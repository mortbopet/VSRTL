#ifndef VSRTL_COMPARATOR_H
#define VSRTL_COMPARATOR_H

#include "vsrtl_component.h"

namespace vsrtl {
namespace core {

#define CMP_COMPONENT(classname, valFunc, op)                                  \
  template <unsigned int W>                                                    \
  class classname : public Component {                                         \
  public:                                                                      \
    classname(const std::string &name, SimComponent *parent)                   \
        : Component(name, parent) {                                            \
      out << [=] { return op1.valFunc() op op2.valFunc(); };                   \
    }                                                                          \
    OUTPUTPORT(out, 1);                                                        \
    INPUTPORT(op1, W);                                                         \
    INPUTPORT(op2, W);                                                         \
  };

CMP_COMPONENT(Sge, sValue, >=)
CMP_COMPONENT(Slt, sValue, <)
CMP_COMPONENT(Uge, uValue, >=)
CMP_COMPONENT(Ult, uValue, <)
CMP_COMPONENT(Eq, uValue, ==)

} // namespace core
} // namespace vsrtl

#endif // VSRTL_COMPARATOR_H
