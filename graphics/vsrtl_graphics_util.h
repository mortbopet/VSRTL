#ifndef VSRTL_GRAPHICS_UTIL_H
#define VSRTL_GRAPHICS_UTIL_H

#include "vsrtl_graphics_defines.h"

#include <QRect>

namespace vsrtl {

// Round up v to nearest multiple of m
inline int roundUp(int v, int m) {
    int remainder = v % m;
    if (remainder == 0)
        return v;
    return v + m - remainder;
}

// Round v to nearest multiple of m (tie: round up)
inline int roundNear(int v, int m) {
    return ((v + m / 2) / m) * m;
}

inline void roundNear(QPointF& p, int m) {
    p.setX(roundNear(p.x(), m));
    p.setY(roundNear(p.y(), m));
}

template <typename RectType>
RectType boundingRectOfRects(const RectType& r1, const RectType& r2) {
    qreal top, bottom, right, left;
    left = r1.left() < r2.left() ? r1.left() : r2.left();
    right = r1.right() > r2.right() ? r1.right() : r2.right();
    top = r1.top() < r2.top() ? r1.top() : r2.top();
    bottom = r1.bottom() > r2.bottom() ? r1.bottom() : r2.bottom();

    RectType r;
    r.setLeft(left);
    r.setRight(right);
    r.setTop(top);
    r.setBottom(bottom);

    return r;
}

template <typename RectType>
RectType boundingRectOfRects(const QList<RectType>& rects) {
    RectType br;
    for (const auto& r : rects) {
        br = boundingRectOfRects(br, r);
    }
    return br;
}

template <typename RectType>
RectType normalizeRect(const RectType& r1) {
    auto r = r1;
    r.setTopLeft(QPointF(0, 0));
    return r;
}

static inline QRectF gridToScene(const QRect& gridRect) {
    // Scales a rectangle in grid coordinates to scene coordinates. -1 because QRect returns width and height offset by
    // 1 for "historical reasons"
    QRectF sceneGridRect;
    sceneGridRect.moveTo(gridRect.topLeft() * GRID_SIZE);
    sceneGridRect.setWidth((gridRect.width() - 1) * GRID_SIZE);
    sceneGridRect.setHeight((gridRect.height() - 1) * GRID_SIZE);
    return sceneGridRect;
}

static inline QRect sceneToGrid(QRectF sceneRect) {
    // Scales a rectangle in scene coordinates to grid coordinates
    sceneRect.setWidth(sceneRect.width() / GRID_SIZE);
    sceneRect.setHeight(sceneRect.height() / GRID_SIZE);

    // When converting to integer-based grid rect, round up to ensure all components are inside
    return QRect(QPoint(0, 0), QSize(std::ceil(sceneRect.width()), std::ceil(sceneRect.height())));
}

}  // namespace vsrtl

#endif  // VSRTL_GRAPHICS_UTIL_H
