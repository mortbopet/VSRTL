#ifndef GRIDCOMPONENT_H
#define GRIDCOMPONENT_H

#include "vsrtl_component.h"

namespace vsrtl {

class GridComponent {
public:
    GridComponent(const Component& c);

private:
    const Component& m_component;
};
}  // namespace vsrtl

#endif  // GRIDCOMPONENT_H
