#ifndef RIPES_CIRCUITHANDLER_H
#define RIPES_CIRCUITHANDLER_H

#include "ripes_architecture.h"
#include "ripes_componentgraphic.h"
#include "ripes_ripesview.h"

namespace ripes {

class CircuitHandler {
public:
    CircuitHandler(RipesView* view);
    void orderSubcomponents(const ComponentGraphic* parent);

private:
    RipesView* m_view;
};
}

#endif  // RIPES_CIRCUITHANDLER_H
