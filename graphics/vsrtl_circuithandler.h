#ifndef VSRTL_CIRCUITHANDLER_H
#define VSRTL_CIRCUITHANDLER_H

#include "vsrtl_design.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_view.h"

namespace vsrtl {

class CircuitHandler {
public:
    CircuitHandler(VSRTLView* view);
    void orderSubcomponents(const ComponentGraphic* parent);

private:
    VSRTLView* m_view;
};
}

#endif  // VSRTL_CIRCUITHANDLER_H
