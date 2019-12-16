#ifndef VSRTL_SHAPE_H
#define VSRTL_SHAPE_H

#include <QPainterPath>
#include <QTransform>
#include <typeindex>
#include <typeinfo>

#include "../interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {

/** Shape
 *  Class for storing the draw shape associated with a Component from the VSRTL library
 */
class ShapeRegister {
    /**
     * @brief The Shape struct
     * Component shapes should be scalable in x- and y direction, but may contain complex shapes such as circles.
     * The Shape struct, and shape generation, thus provides an interface for generating (scaling) a QPainterPath,
     * without using QPainte's scale functionality.
     * We avoid using QPainter::scale, given that this also scales the pen, yielding invalid drawings.
     */
public:
    struct Shape {
        std::function<QPainterPath(QTransform)> shapeFunc;
        QRect min_rect;
    };

    static void registerComponentShape(std::type_index component, Shape shape) {
        Q_ASSERT(!s_componentShapes.count(component));
        Q_ASSERT(shape.min_rect.topLeft() == QPoint(0, 0));

        // Ensure that minimum rectangle is snapping to the grid
        s_componentShapes[component] = shape;
    }

    static QPainterPath getComponentShape(std::type_index component, QTransform transform) {
        // If no shape has been registered for the base component type, revert to displaying as a "SimComponent"
        if (!s_componentShapes.count(component)) {
            return s_componentShapes[GraphicsIDFor(Component)].shapeFunc(transform);
        }
        return s_componentShapes[component].shapeFunc(transform);
    }

    static QRect getComponentMinGridRect(std::type_index component) {
        // If no shape has been registered for the base component type, revert to displaying as a "SimComponent"
        if (!s_componentShapes.count(component)) {
            return s_componentShapes[GraphicsIDFor(Component)].min_rect;
        }
        return s_componentShapes[component].min_rect;
    }

private:
    static std::map<std::type_index, Shape> s_componentShapes;
};

}  // namespace vsrtl
#endif  // VSRTL_SHAPE_H
