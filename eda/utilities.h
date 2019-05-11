#ifndef UTILITIES_H
#define UTILITIES_H

#include <QRect>

namespace vsrtl {
namespace eda {
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

template <typename PointType>
inline void roundNear(PointType& p, int m) {
    p.setX(roundNear(static_cast<int>(p.x()), m));
    p.setY(roundNear(static_cast<int>(p.y()), m));
}

template <typename T>
T boundingRect(const T& r1, const T& r2) {
    auto left = r1.left() < r2.left() ? r1.left() : r2.left();
    auto right = r1.right() > r2.right() ? r1.right() : r2.right();
    auto top = r1.top() < r2.top() ? r1.top() : r2.top();
    auto bottom = r1.bottom() > r2.bottom() ? r1.bottom() : r2.bottom();

    T r;
    r.setLeft(left);
    r.setRight(right);
    r.setTop(top);
    r.setBottom(bottom);

    return r;
}

template <typename T, template <typename, typename = std::allocator<T>> class Container>
T boundingRectOfRects(const Container<T>& rects) {
    T br;
    for (const auto& r : rects) {
        br = boundingRect(br, r);
    }
    return br;
}

template <typename RectType>
RectType normalizeRect(const RectType& r1) {
    auto r = r1;
    r.setTopLeft(QPointF(0, 0));
    return r;
}
}  // namespace eda
}  // namespace vsrtl

#endif  // UTILITIES_H
