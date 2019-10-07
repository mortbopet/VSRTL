#ifndef VSRTL_WIREGRAPHIC_H
#define VSRTL_WIREGRAPHIC_H

#include "vsrtl_graphicsbase.h"

#include <QPen>
#include <memory>

namespace vsrtl {

class PortGraphic;
class Port;
class WireGraphic;
class WireSegment;

/**
 * @brief The PointGraphic class
 * Base class for wire graphic points.
 */
class PointGraphic : public GraphicsBase {
public:
    PointGraphic(QGraphicsItem* parent);
    QRectF boundingRect() const override { return QRectF(); }
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) {}
};

/**
 * @brief The WirePoint class
 * A point on a wire. Shall display a context menu to delete. is select- and moveable.
 */
class WirePoint : public PointGraphic {
public:
    WirePoint(WireSegment& parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    WireSegment& m_parent;
};

class WireSegment : public GraphicsBase {
public:
    WireSegment(WireGraphic& parent);

    void setStart(PointGraphic* start) { m_start = start; }
    void setEnd(PointGraphic* end) { m_end = end; }

    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    QLineF getLine() const;

    PointGraphic* m_start = nullptr;
    PointGraphic* m_end = nullptr;
    WireGraphic& m_parent;
};

class WireGraphic : public GraphicsBase {
    friend class PortGraphic;

public:
    WireGraphic(PortGraphic* from, const std::vector<Port*>& to, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    const QPen& getPen();
    void postSceneConstructionInitialize1() override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override {}

    PortGraphic* getFromPort() const { return m_fromPort; }
    const std::vector<PortGraphic*>& getToPorts() const { return m_toGraphicPorts; }

private:
    PortGraphic* m_fromPort = nullptr;
    const std::vector<Port*>& m_toPorts;
    std::vector<PortGraphic*> m_toGraphicPorts;
    std::vector<std::unique_ptr<WireSegment>> m_segments;
};
}  // namespace vsrtl

#endif  // VSRTL_WIREGRAPHIC_H
