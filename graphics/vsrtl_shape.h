#ifndef VSRTL_SHAPE_H
#define VSRTL_SHAPE_H

#include <QPainterPath>

namespace vsrtl {

namespace core {
class Component;
}

/** Shape
 *  Class for storing the draw shape associated with a Component from the VSRTL library
 */
template <typename T>
class Shape {
public:
    static void setShape(const QPainterPath& shape) { getInstance().m_shape = shape; }
    static const QPainterPath& getShape() {
        const auto& shape = getInstance().m_shape;
        return shape;
    }

private:
    static Shape<T>& getInstance() {
        static Shape<T> instance;
        return instance;
    }

    // Default shape is a square
    Shape() { m_shape.addPolygon(QPolygonF({{0, 0}, {1, 0}, {1, 1}, {0, 1}})); }
    static QPainterPath m_shape;
};

}  // namespace vsrtl
#endif  // VSRTL_SHAPE_H
