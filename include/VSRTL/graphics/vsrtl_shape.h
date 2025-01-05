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
 *  Class for storing the draw shape associated with a Component from the VSRTL
 * library
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
   * Component shapes should be scalable in x- and y direction, but may contain
   * complex shapes such as circles. The Shape struct, and shape generation,
   * thus provides an interface for generating (scaling) a QPainterPath, without
   * using QPainte's scale functionality. We avoid using QPainter::scale, given
   * that this also scales the pen, yielding invalid drawings.
   */
  struct Shape {
    std::function<QPainterPath(QTransform)> shapeFunc;
    QRect min_rect = QRect();
  };

  static QPainterPath getTypeShape(const GraphicsType *type,
                                   const QTransform &transform) {
    // If no shape has been registered for the base component type, revert to
    // displaying as a "SimComponent"
    if (!get().m_typeShapes.count(type)) {
      return get().m_typeShapes[GraphicsTypeFor(Component)].shapeFunc(
          transform);
    }
    return get().m_typeShapes[type].shapeFunc(transform);
  }

  static QRect getTypePreferredRect(const GraphicsType *type) {
    // If no shape has been registered for the base component type, revert to
    // displaying as a "SimComponent"
    if (!get().m_typeShapes.count(type)) {
      return get().m_typeShapes[GraphicsTypeFor(Component)].min_rect;
    }
    return get().m_typeShapes[type].min_rect;
  }

private:
  static ShapeRegister &get() {
    static ShapeRegister sr;
    return sr;
  }

  void registerTypeShape(const GraphicsType *type, const Shape &shape) {
    Q_ASSERT(!m_typeShapes.count(type));
    Q_ASSERT(shape.min_rect.topLeft() == QPoint(0, 0));

    // Ensure that minimum rectangle is snapping to the grid
    m_typeShapes[type] = shape;
  }

  std::map<const GraphicsType *, Shape> m_typeShapes;
};

} // namespace vsrtl
#endif // VSRTL_SHAPE_H
