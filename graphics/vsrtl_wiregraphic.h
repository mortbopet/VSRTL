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
class ComponentGraphic;

/**
 * @brief The PointGraphic class
 * Base class for wire graphic points.
 */
class PointGraphic : public GraphicsBase {
public:
    PointGraphic(QGraphicsItem* parent);
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
};

/**
 * @brief The WirePoint class
 * A point on a wire. Shall display a context menu to delete. is select- and moveable.
 */
class WirePoint : public PointGraphic {
public:
    WirePoint(WireGraphic& parent, QGraphicsItem* sceneParent);

    void addOutputWire(WireSegment* wire) { m_outputWires.push_back(wire); }
    std::vector<WireSegment*>& getOutputWires() { return m_outputWires; }
    void setInputWire(WireSegment* wire) { m_inputWire = wire; }
    WireSegment* getInputWire() { return m_inputWire; }

    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    WireGraphic& m_parent;
    ComponentGraphic* m_sceneParent;
    WireSegment* m_inputWire = nullptr;
    std::vector<WireSegment*> m_outputWires;
};

class WireSegment : public GraphicsBase {
public:
    WireSegment(WireGraphic* parent);

    void setStart(PointGraphic* start) { m_start = start; }
    void setEnd(PointGraphic* end) { m_end = end; }
    PointGraphic* getStart() const { return m_start; }
    PointGraphic* getEnd() const { return m_end; }

    QPainterPath shape() const override;
    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    QString deleted = "False";

private:
    QLineF getLine() const;

    PointGraphic* m_start = nullptr;
    PointGraphic* m_end = nullptr;
    WireGraphic* m_parent;
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
    void addWirePoint(const QPointF scenePos, WireSegment* onSegment);
    void removeWirePoint(WirePoint* point);

private:
    PortGraphic* m_fromPort = nullptr;
    const std::vector<Port*>& m_toPorts;
    std::vector<PortGraphic*> m_toGraphicPorts;
    std::vector<WireSegment*> m_wires;
    std::vector<WirePoint*> m_points;
};
}  // namespace vsrtl

#endif  // VSRTL_WIREGRAPHIC_H
