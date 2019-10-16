#include "vsrtl_wiregraphic.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_port.h"
#include "vsrtl_portgraphic.h"

#include "vsrtl_graphics_defines.h"

#include <QGraphicsScene>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QPolygon>

#include <math.h>

namespace vsrtl {

PointGraphic::PointGraphic(QGraphicsItem* parent) {
    setParentItem(parent);
}

QRectF PointGraphic::boundingRect() const {
#ifdef VSRTL_DEBUG_DRAW
    return QRectF(-WIRE_WIDTH / 2, -WIRE_WIDTH / 2, WIRE_WIDTH / 2, WIRE_WIDTH / 2);
#else
    return QRectF();
#endif
}

void PointGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) {
#ifdef VSRTL_DEBUG_DRAW
    DRAW_BOUNDING_RECT(painter)
    painter->save();
    QPen pen(Qt::red);
    pen.setWidth(5);
    painter->setPen(pen);
    painter->drawPoint(QPointF(0, 0));
    painter->restore();
#endif
}

void WirePoint::pointDrop(WirePoint* point) {
    if (m_parent.canMergePoints(this, point) == WireGraphic::MergeType::CannotMerge)
        return;

    m_parent.mergePoints(this, point);
}

void WirePoint::pointDragEnter(WirePoint* point) {
    if (m_parent.canMergePoints(this, point) != WireGraphic::MergeType::CannotMerge) {
        m_draggedOnThis = point;
        update();
    }
}

void WirePoint::pointDragLeave(WirePoint*) {
    m_draggedOnThis = nullptr;
    update();
}

WirePoint::WirePoint(WireGraphic& parent, QGraphicsItem* sceneParent) : PointGraphic(sceneParent), m_parent(parent) {
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);
    m_sceneParent = dynamic_cast<ComponentGraphic*>(sceneParent);
    Q_ASSERT(m_sceneParent != nullptr);
}

QRectF WirePoint::boundingRect() const {
    return shape().boundingRect().adjusted(-WIRE_WIDTH, -WIRE_WIDTH, WIRE_WIDTH, WIRE_WIDTH);
}

QVariant WirePoint::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemPositionChange) {
        // Snap to grid
        QPointF newPos = value.toPointF();
        qreal x = round(newPos.x() / GRID_SIZE) * GRID_SIZE;
        qreal y = round(newPos.y() / GRID_SIZE) * GRID_SIZE;
        return QPointF(x, y);
    } else if (change == QGraphicsItem::ItemPositionHasChanged) {
        // Propagate a redraw call to all segments connecting to this
        m_inputWire->prepareGeometryChange();
        for (const auto& wire : m_outputWires)
            wire->prepareGeometryChange();
    }

    return QGraphicsItem::itemChange(change, value);
}

QPainterPath WirePoint::shape() const {
    QPainterPath path;
    path.addEllipse({0, 0}, WIRE_WIDTH, WIRE_WIDTH);
    path.setFillRule(Qt::WindingFill);
    return path;
}

void WirePoint::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    // Given that WirePoints are graphical children of a component graphic (for them to be fixed on the component),
    // they will still be drawn when the component graphic is not expanded (given that it is still visible). Thus,
    // only paint if the parent componentgraphic is expanded.
    if (!m_sceneParent->isExpanded())
        return;

    painter->save();
    QPen pen = m_parent.getPen();
    painter->setPen(pen);
    painter->drawPath(shape());
    if (m_draggedOnThis != nullptr) {
        pen.setColor(Qt::red);
        painter->setPen(pen);
        painter->setBrush(Qt::transparent);
        painter->drawRect(
            shape().boundingRect().adjusted(-WIRE_WIDTH / 2, -WIRE_WIDTH / 2, WIRE_WIDTH / 2, WIRE_WIDTH / 2));
    }
    painter->restore();
}

void WirePoint::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    auto removePointAction = menu.addAction("Remove point");
    connect(removePointAction, &QAction::triggered, [this](bool) { m_parent.removeWirePoint(this); });
    menu.exec(event->screenPos());
}

// --------------------------------------------------------------------------------

WireSegment::WireSegment(WireGraphic* parent) : m_parent(parent) {
    setParentItem(parent);
    setAcceptHoverEvents(true);
}

QLineF WireSegment::getLine() const {
    if (m_start == nullptr || m_end == nullptr)
        return QLine();

    const auto& startPos = mapFromItem(m_start->parentItem(), m_start->pos());
    const auto& endPos = mapFromItem(m_end->parentItem(), m_end->pos());
    return QLineF(startPos, endPos);
}

/**
 * @brief expandLine
 * Given a line, returns a box of width 2xsideWidth around the line
 */
QPolygonF expandLine(const QLineF line, const qreal sideWidth) {
    const auto norm1 = line.normalVector();
    const auto norm2 = norm1.translated(line.p2() - line.p1());

    const auto lineWidthParam = qreal(sideWidth) / line.length();

    const QPointF rp1 = norm1.pointAt(lineWidthParam);
    const QPointF rp2 = norm2.pointAt(lineWidthParam);
    const QPointF rp3 = norm2.pointAt(-lineWidthParam);
    const QPointF rp4 = norm1.pointAt(-lineWidthParam);
    return QPolygonF({rp1, rp2, rp3, rp4});
}

QPainterPath WireSegment::shape() const {
    if (m_start == nullptr || m_end == nullptr)
        return QPainterPath();

    QPainterPath path;
    path.addPolygon(expandLine(getLine(), WIRE_WIDTH));
    return path;
}

QRectF WireSegment::boundingRect() const {
    if (m_start == nullptr || m_end == nullptr)
        return QRectF();

    auto br = shape().boundingRect();
    br.adjust(-WIRE_WIDTH, -WIRE_WIDTH, WIRE_WIDTH, WIRE_WIDTH);
    return br;
}

void WireSegment::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    setToolTip(m_parent->getFromPort()->getTooltipString());
}

void WireSegment::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    if (m_start == nullptr || m_end == nullptr)
        return;
    if (!m_start->isVisible() || !m_end->isVisible())
        return;

    painter->save();
    painter->setPen(m_parent->getPen());
    painter->drawLine(getLine());
    painter->restore();
#ifdef VSRTL_DEBUG_DRAW
    DRAW_BOUNDING_RECT(painter)
#endif
}

void WireSegment::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    // Todo: allow adding points. Should possibly just be done by a click on the wire
    QMenu menu;
    auto createPointAction = menu.addAction("Add wire point");
    connect(createPointAction, &QAction::triggered,
            [this, &event](bool) { m_parent->addWirePoint(event->scenePos(), this); });
    menu.exec(event->screenPos());
}

void WireSegment::mousePressEvent(QGraphicsSceneMouseEvent* event) {}

// --------------------------------------------------------------------------------

void WireGraphic::addWirePoint(const QPointF scenePos, WireSegment* onSegment) {
    const auto& endPoint = onSegment->getEnd();

    // Create new point managed by this graphic.
    // The parent structure is somewhat nested; a wireGraphic must be a graphical child of the graphics component
    // which this wire is nested inside. To reach this, we do: WireGraphic->parent = PortGraphic PortGraphic->parent
    // = ComponentGraphic ComponentGrahic -> parent = ComponentGraphic (with nested components)
    auto* topComponent = parentItem()->parentItem()->parentItem();
    Q_ASSERT(topComponent != nullptr);
    auto* newPoint = new WirePoint(*this, topComponent);

    // Set the wire segment of which this point was created, to terminate at the new point
    onSegment->setEnd(newPoint);

    // Create wire segment between new point and previous end point
    auto* newSeg = new WireSegment(this);
    newSeg->setStart(newPoint);
    newSeg->setEnd(endPoint);

    // Register the wire segments with the point
    newPoint->setInputWire(onSegment);
    newPoint->addOutputWire(newSeg);

    // Make the incoming wire to the end point the newly created wire
    if (auto* endPointWire = dynamic_cast<WirePoint*>(endPoint))
        endPointWire->setInputWire(newSeg);

    // Move new point to its creation position
    newPoint->setPos(mapToItem(topComponent, mapFromScene(scenePos)));

    // Add newly created objects to objects managed by this wiregraphic
    m_wires.insert(newSeg);
    m_points.insert(newPoint);
}

void WireGraphic::removeWirePoint(WirePoint* pointToRemove) {
    Q_ASSERT(std::find(m_points.begin(), m_points.end(), pointToRemove) != m_points.end());
    auto* wireToRemove = pointToRemove->getInputWire();
    auto* newStartPoint = wireToRemove->getStart();
    Q_ASSERT(newStartPoint != nullptr);
    Q_ASSERT(newStartPoint != pointToRemove);
    auto* newStartWirePoint = dynamic_cast<WirePoint*>(newStartPoint);
    // Set all wires previously connecting to the input wire to connect with the new start point
    for (auto& wire : pointToRemove->getOutputWires()) {
        wire->setStart(newStartPoint);
        if (newStartWirePoint != nullptr) {
            newStartWirePoint->addOutputWire(wire);
        }
    }

    // Delete the (now defunct) wire between the new start point and the point to be removed

    auto iter = std::find(m_wires.begin(), m_wires.end(), wireToRemove);
    Q_ASSERT(std::find(pointToRemove->getOutputWires().begin(), pointToRemove->getOutputWires().end(), wireToRemove) ==
             pointToRemove->getOutputWires().end());
    Q_ASSERT(iter != m_wires.end());
    (*iter)->setEnd(nullptr);
    (*iter)->setStart(nullptr);
    m_wires.erase(iter);

    // Deregister the wire to remove with its old starting point
    if (newStartWirePoint != nullptr) {
        auto& outputWires = newStartWirePoint->getOutputWires();
        auto iter = std::find(outputWires.begin(), outputWires.end(), wireToRemove);
        Q_ASSERT(iter != outputWires.end());
        outputWires.erase(iter);
    }

    // Mark the wire for deletion
    wireToRemove->deleteLater();

    // Finally, delete the point
    auto p_iter = std::find(m_points.begin(), m_points.end(), pointToRemove);
    Q_ASSERT(p_iter != m_points.end());
    m_points.erase(p_iter);
    pointToRemove->deleteLater();
}

WireGraphic::WireGraphic(PortGraphic* from, const std::vector<PortBase*>& to, QGraphicsItem* parent)
    : m_fromPort(from), m_toPorts(to) {
    setParentItem(parent);
}

bool WireGraphic::managesPoint(WirePoint* point) const {
    return std::find(m_points.begin(), m_points.end(), point) != m_points.end();
}

void WireGraphic::mergePoints(WirePoint* base, WirePoint* toMerge) {
    const auto mergeType = canMergePoints(base, toMerge);
    Q_ASSERT(mergeType != MergeType::CannotMerge);

    for (const auto& wire : toMerge->getOutputWires()) {
        wire->setStart(base);
        base->addOutputWire(wire);
    }

    // With all wires moved to their new positions, the merge point may be removed through the usual remove logic
    toMerge->clearOutputWires();
    if (mergeType == MergeType::MergeSourceWithSink)
        toMerge->addOutputWire(base->getInputWire());
    removeWirePoint(toMerge);
}

/**
 * @brief WireGraphic::canMergePoints
 * Points which are adjacent may be merged.
 * Points which both have their input wires connected to the source port may be merged.
 */
WireGraphic::MergeType WireGraphic::canMergePoints(WirePoint* base, WirePoint* toMerge) const {
    if (!managesPoint(base) || !managesPoint(toMerge))
        return MergeType::CannotMerge;

    auto* baseptr = static_cast<PointGraphic*>(base);

    // is toMerge fed by base?
    if (baseptr == toMerge->getInputWire()->getStart())
        return MergeType::MergeSinkWithSource;

    // does toMerge feed into base?
    for (const auto& wire : toMerge->getOutputWires()) {
        if (wire->getEnd() == baseptr)
            return MergeType::MergeSourceWithSink;
    }

    // is toMerge and base fed by the same source port?
    if (base->getInputWire()->getStart() == toMerge->getInputWire()->getStart())
        return MergeType::MergeParallelSinks;

    return MergeType::CannotMerge;
}

QRectF WireGraphic::boundingRect() const {
    QPolygonF p;
    p.append(mapFromItem(m_fromPort, m_fromPort->getInputPoint()));
    for (const auto& to : m_toGraphicPorts) {
        p.append(mapFromItem(to, to->getInputPoint()));
    }
    QRectF br = p.boundingRect();
    br.adjust(-WIRE_WIDTH, -WIRE_WIDTH, WIRE_WIDTH, WIRE_WIDTH);

    // HACK HACK HACK
    // To ensure that input ports are redrawn when this wire changes (ie. gets selected), we overlap the bounding
    // rect of this item onto both of its ports, ensuring redraws
    br.adjust(-parentItem()->boundingRect().width(), 0, parentItem()->boundingRect().width(), 0);
    // HACK HACK HACK

    return br;
}

/**
 * @brief WireGraphic::postSceneConstructionInitialize1
 * With all ports and components created during circuit construction, wires may now register themselves with their
 * attached input- and output ports
 */
void WireGraphic::postSceneConstructionInitialize1() {
    for (const auto& item : scene()->items()) {
        PortGraphic* portItem = dynamic_cast<PortGraphic*>(item);
        if (portItem) {
            if (std::find(m_toPorts.begin(), m_toPorts.end(), portItem->getPort()) != m_toPorts.end()) {
                m_toGraphicPorts.push_back(portItem);
            }
        }
        if (m_toGraphicPorts.size() == m_toPorts.size()) {
            break;
        }
    }

    // Assert that all ports were found in the scene
    Q_ASSERT(m_toGraphicPorts.size() == m_toPorts.size());

    // Make the wire destination ports aware of this wire
    for (const auto& sink : m_toGraphicPorts) {
        sink->setInputWire(this);

        // Create wire segment between source and sink port
        auto* seg = new WireSegment(this);
        seg->setStart(m_fromPort->getPointGraphic());
        seg->setEnd(sink->getPointGraphic());
        m_wires.insert(seg);
    }

    GraphicsBase::postSceneConstructionInitialize1();
}

const QPen& WireGraphic::getPen() {
    // propagate the source port pen to callers
    return m_fromPort->getPen();
}

}  // namespace vsrtl
