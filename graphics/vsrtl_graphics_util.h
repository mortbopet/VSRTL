#ifndef VSRTL_GRAPHICS_UTIL_H
#define VSRTL_GRAPHICS_UTIL_H

#include <QRect>

#include <QGraphicsItem>

#include "vsrtl_graphics_serializers.h"

namespace vsrtl {

// Round up v to nearest multiple of m
inline int roundUp(int v, int m) {
    int remainder = v % m;
    if (remainder == 0)
        return v;
    return v + m - remainder;
}

template <typename RectType>
inline void scaleToGrid(RectType& r, int gridsize) {
    r.setWidth(r.width() * gridsize);
    r.setHeight(r.height() * gridsize);
}

// Round v to nearest multiple of m (tie: round up)
inline int roundNear(int v, int m) {
    return ((v + m / 2) / m) * m;
}

inline void roundNear(QPointF& p, int m) {
    p.setX(roundNear(p.x(), m));
    p.setY(roundNear(p.y(), m));
}

template <typename R, typename T, typename F>
std::vector<R> collect(T list, F func) {
    std::vector<R> r;
    for (const auto& i : list) {
        r.push_back((i->*func)());
    }
    return r;
}

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
bool snapRectToRect(const RectType& bound, RectType& snapping) {
    bool snap_r, snap_b;
    snap_r = false;
    snap_b = false;

    if (snapping.right() < bound.right()) {
        snapping.setRight(bound.right());
        snap_r = true;
    }
    if (snapping.bottom() < bound.bottom()) {
        snapping.setBottom(bound.bottom());
        snap_b = true;
    }

    return !(snap_r & snap_b);
}

template <typename RectType>
RectType boundingRectOfRects(const std::vector<RectType>& rects) {
    RectType boundingRect = rects.at(0);
    for (int i = 1; i < rects.size(); i++) {
        boundingRect = boundingRectOfRects<RectType>(boundingRect, rects.at(i));
    }
    return boundingRect;
}

template <typename RectType>
RectType normalizeRect(const RectType& r1) {
    auto r = r1;
    r.setTopLeft(QPointF(0, 0));
    return r;
}

inline void getAllChildren(QGraphicsItem* p, QList<QGraphicsItem*>& acc) {
    if (p->childItems().size() == 0) {
        // Leaf
        acc.push_back(p);
    } else {
        for (const auto& c : p->childItems())
            getAllChildren(c, acc);
        acc.push_back(p);
    }
}

}  // namespace vsrtl

#endif  // VSRTL_GRAPHICS_UTIL_H
