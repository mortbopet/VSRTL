#include "componentshape.h"
#include "core/vsrtl_component.h"

#include <map>

namespace vsrtl {
namespace eda {
namespace {
static std::map<std::type_index, Shape> s_componentShapes;
}

void setComponentShape(std::type_index component, Shape shape) {
    Q_ASSERT(s_componentShapes.count(component) == 0);
    Q_ASSERT(shape.min_rect.topLeft() == QPoint(0, 0));

    // Ensure that minimum rectangle is snapping to the grid
    s_componentShapes[component] = shape;
}

QPainterPath getComponentShape(std::type_index component, QTransform transform) {
    // If no shape has been registered for the base component type, revert to displaying as a "Component"
    if (s_componentShapes.count(component) == 0) {
        return s_componentShapes[typeid(Component)].shapeFunc(transform);
    }
    return s_componentShapes[component].shapeFunc(transform);
}

QRect getComponentMinGridRect(std::type_index component) {
    // If no shape has been registered for the base component type, revert to displaying as a "Component"
    if (s_componentShapes.count(component) == 0) {
        return s_componentShapes[typeid(Component)].min_rect;
    }
    return s_componentShapes[component].min_rect;
}

}  // namespace eda
}  // namespace vsrtl
