#ifndef VSRTL_SHAPE_H
#define VSRTL_SHAPE_H

#include <QPainterPath>
#include <QTransform>
#include <functional>
#include <typeindex>
#include <typeinfo>

#include "../interface/vsrtl_gfxobjecttypes.h"

namespace vsrtl {

/** Shape
 *  Class for storing the draw shape associated with a Component from the VSRTL library
 */
class ShapeRegister {
private:
    /**
     * @brief ShapeRegister
     * Constructor shall initialize all componentShapes
     */
    ShapeRegister();

public:
    /**
     * @brief The Shape struct
     * Component shapes should be scalable in x- and y direction, but may contain complex shapes such as circles.
     * The Shape struct, and shape generation, thus provides an interface for generating (scaling) a QPainterPath,
     * without using QPainte's scale functionality.
     * We avoid using QPainter::scale, given that this also scales the pen, yielding invalid drawings.
     */
    struct Shape {
        std::function<QPainterPath(QTransform)> shapeFunc;
        QRect min_rect = QRect();
    };

    static QPainterPath getComponentShape(const std::type_index& component, const QTransform& transform) {
        // If no shape has been registered for the base component type, revert to displaying as a "SimComponent"
        if (!get().m_componentShapes.count(component)) {
            return get().m_componentShapes[GraphicsIDFor(Component)].shapeFunc(transform);
        }
        return get().m_componentShapes[component].shapeFunc(transform);
    }

    static QRect getComponentPreferredRect(const std::type_index& component) {
        // If no shape has been registered for the base component type, revert to displaying as a "SimComponent"
        if (!get().m_componentShapes.count(component)) {
            return get().m_componentShapes[GraphicsIDFor(Component)].min_rect;
        }
        return get().m_componentShapes[component].min_rect;
    }

private:
    static ShapeRegister& get() {
        static ShapeRegister sr;
        return sr;
    }

    void registerComponentShape(const std::type_index& component, const Shape& shape) {
        Q_ASSERT(!m_componentShapes.count(component));
        Q_ASSERT(shape.min_rect.topLeft() == QPoint(0, 0));

        // Ensure that minimum rectangle is snapping to the grid
        m_componentShapes[component] = shape;
    }

    std::map<std::type_index, Shape> m_componentShapes;
};

}  // namespace vsrtl
#endif  // VSRTL_SHAPE_H
