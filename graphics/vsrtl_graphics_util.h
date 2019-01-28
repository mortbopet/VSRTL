#ifndef VSRTL_GRAPHICS_UTIL_H
#define VSRTL_GRAPHICS_UTIL_H

#include <QRect>

namespace vsrtl {

template <typename RectType>
RectType boundingRectOfRects(const RectType& r1, const RectType& r2) {
    qreal top, bottom, right, left;
    left = r1.left() < r2.left() ? r1.left() : r2.left();
    right = r1.right() > r2.right() ? r1.right() : r2.right();
    top = r1.top() < r2.top() ? r1.top() : r2.top();
    bottom = r1.bottom() > r2.bottom() ? r1.bottom() : r2.bottom();

    return RectType(left, top, right, bottom);
}

template <typename RectType>
RectType normalizeRect(const RectType& r1) {
    auto r = r1;
    r.setTopLeft(QPointF(0, 0));
    return r;
}
}

#endif  // VSRTL_GRAPHICS_UTIL_H
