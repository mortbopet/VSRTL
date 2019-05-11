#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QPoint>
#include <QRect>

namespace vsrtl {
namespace eda {

enum class Edge { Top, Bottom, Left, Right };
enum class Direction { Horizontal, Vertical };
enum class Corner { TopLeft, TopRight, BottomRight, BottomLeft };
enum class IntersectType { Cross, OnEdge };


/**
 * @brief The Line class
 * Simple orthogonal line class with integer coordinates. Similar to QLine, however, this does not carry the strange
 * "historical" artifacts which are present in QLine.
 */
class Line {
public:

    Line(QPoint p1, QPoint p2) {
        // Assert that the line is orthogonal to one of the grid axis
        Q_ASSERT(p1.x() == p2.x() || p1.y() == p2.y());
        m_p1 = p1;
        m_p2 = p2;

        m_orientation = p1.x() == p2.x() ? Direction::Vertical : Direction::Horizontal;
    }

    const QPoint& p1() const { return m_p1; }
    const QPoint& p2() const { return m_p2; }
    void setP1(const QPoint& p) { m_p1 = p; }
    void setP2(const QPoint& p) { m_p2 = p; }
    Direction orientation() const { return m_orientation; }
    bool intersect(const Line& other, QPoint& p, IntersectType type) const {
        Q_ASSERT(orientation() != other.orientation());
        const Line *hz, *vt;
        if (m_orientation == Direction::Horizontal) {
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

    bool operator==(const Line& rhs) const { return m_p1 == rhs.m_p1 && m_p2 == rhs.m_p2; }

private:
    QPoint m_p1;
    QPoint m_p2;
    Direction m_orientation;
};

static inline Line getEdge(const QRect& rect, Edge e) {
    switch (e) {
        case Edge::Top: {
            return Line(rect.topLeft(), rect.topRight());
        }
        case Edge::Bottom: {
            return Line(rect.bottomLeft(), rect.bottomRight());
        }
        case Edge::Left: {
            return Line(rect.topLeft(), rect.bottomLeft());
        }
        case Edge::Right: {
            return Line(rect.topRight(), rect.bottomRight());
        }
    }
}

}
}

#endif // GEOMETRY_H
