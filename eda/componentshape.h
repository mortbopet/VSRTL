#ifndef COMPONENTSHAPE_H
#define COMPONENTSHAPE_H

#include <QPainterPath>
#include <QRect>
#include <QTransform>

#include <typeindex>
#include <functional>

namespace vsrtl {
namespace eda {

/**
 * @brief The Shape struct
 * Component shapes should be scalable in x- and y direction, but may contain complex shapes such as circles.
 * The Shape struct, and shape generation, thus provides an interface for generating (scaling) a QPainterPath,
 * without using QPainte's scale functionality.
 * We avoid using QPainter::scale, given that this also scales the pen, yielding invalid drawings.
 */

struct Shape {
    std::function<QPainterPath(QTransform)> shapeFunc;
    QRect min_rect;
};

void setComponentShape(std::type_index component, Shape shape);
QPainterPath getComponentShape(std::type_index component, QTransform transform);
QRect getComponentMinGridRect(std::type_index component);

}
}

#endif // COMPONENTSHAPE_H
