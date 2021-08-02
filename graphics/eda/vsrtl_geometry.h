#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QLine>
#include <QPoint>
#include <QRect>

#include <set>

namespace vsrtl {
enum Direction { North, South, West, East, NDirections };
#define allDirections (std::set<Direction>{Direction::North, Direction::South, Direction::West, Direction::East})
enum class Orientation { Horizontal, Vertical };
enum class Corner { TopLeft, TopRight, BottomRight, BottomLeft };

namespace eda {
enum class IntersectType { Cross, OnEdge };

inline Direction inv(Direction dir) {
    switch (dir) {
        case North:
            return South;
        case South:
            return North;
        case West:
            return East;
        case East:
            return West;
        case NDirections:
            assert(false);
    }
    Q_UNREACHABLE();
}

inline Orientation directionToOrientation(const Direction e) {
    switch (e) {
        case Direction::North:
        case Direction::South:
            return Orientation::Vertical;
        case Direction::West:
        case Direction::East:
            return Orientation::Horizontal;
        case Direction::NDirections:
            assert(false);
    }
    Q_UNREACHABLE();
}

inline std::set<Direction> orientationToDirections(const Orientation o) {
    switch (o) {
        case Orientation::Horizontal:
            return {Direction::East, Direction::West};
        case Orientation::Vertical:
            return {Direction::North, Direction::South};
    }
    Q_UNREACHABLE();
}

/**
 * @brief The Line class
 * Simple orthogonal line class with integer coordinates. Similar to QLine, however, this does not carry the strange
 * "historical" artifacts which are present in QLine (see QLine documentation)
 */
class Line {
public:
    constexpr Line(const QPoint& p1, const QPoint& p2)
        : m_p1(p1), m_p2(p2), m_orientation(p1.x() == p2.x() ? Orientation::Vertical : Orientation::Horizontal) {
        // Assert that the line is orthogonal to one of the grid axis
        Q_ASSERT(p1.x() == p2.x() || p1.y() == p2.y());
    }

    constexpr const QPoint& p1() const { return m_p1; }
    constexpr const QPoint& p2() const { return m_p2; }
    constexpr void setP1(const QPoint& p) { m_p1 = p; }
    constexpr void setP2(const QPoint& p) { m_p2 = p; }
    constexpr Orientation orientation() const { return m_orientation; }
    Line adjusted(const QPoint& dp1, const QPoint& dp2) const { return Line(p1() + dp1, p2() + dp2); }
    QLine toQLine() const { return QLine(p1(), p2()); }
    bool intersect(const Line& other, QPoint& p, IntersectType type) const {
        Q_ASSERT(orientation() != other.orientation());
        const Line *hz, *vt;
        if (m_orientation == Orientation::Horizontal) {
            hz = this;
            vt = &other;
        } else {
            hz = &other;
            vt = this;
        }

        // Assert that the lines are orthogonal
        Q_ASSERT(hz->p1().y() == hz->p2().y());
        Q_ASSERT(vt->p2().x() == vt->p2().x());

        bool hz_intersect, vt_intersect;
        if (type == IntersectType::Cross) {
            // Lines must cross before an intersection is registered
            hz_intersect = (hz->p1().x() < vt->p1().x()) && (vt->p1().x() < hz->p2().x());
            vt_intersect = (vt->p1().y() < hz->p1().y()) && (hz->p1().y() < vt->p2().y());
        } else {
            // Lines may terminate on top of another for an intersection to register
            hz_intersect = (hz->p1().x() <= vt->p1().x()) && (vt->p1().x() <= hz->p2().x());
            vt_intersect = (vt->p1().y() <= hz->p1().y()) && (hz->p1().y() <= vt->p2().y());
        }

        if (hz_intersect && vt_intersect) {
            p = QPoint(vt->p1().x(), hz->p1().y());
            return true;
        }
        p = QPoint();
        return false;
    }

    constexpr bool operator==(const Line& rhs) const { return m_p1 == rhs.m_p1 && m_p2 == rhs.m_p2; }

private:
    QPoint m_p1;
    QPoint m_p2;
    Orientation m_orientation;
};

// The following functions fixes the "historical reason" for incorrect corner coordinates in QRect (see QRect
// documentation).
inline QPoint realTopRight(const QRect& rect) {
    return rect.topRight() + QPoint(1, 0);
}

inline QPoint realBottomRight(const QRect& rect) {
    return rect.bottomRight() + QPoint(1, 1);
}
inline QPoint realBottomLeft(const QRect& rect) {
    return rect.bottomLeft() + QPoint(0, 1);
}

inline Line getEdge(const QRect& rect, Direction e) {
    switch (e) {
        case Direction::North:
            return Line(rect.topLeft(), realTopRight(rect));
        case Direction::South:
            return Line(realBottomLeft(rect), realBottomRight(rect));
        case Direction::West:
            return Line(rect.topLeft(), realBottomLeft(rect));
        case Direction::East:
            return Line(realTopRight(rect), realBottomRight(rect));
        case Direction::NDirections:
            assert(false);
    }
    Q_UNREACHABLE();
}

}  // namespace eda
}  // namespace vsrtl

#endif  // GEOMETRY_H
