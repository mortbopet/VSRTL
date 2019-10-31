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
#include <QStyleOptionGraphicsItem>

#include <math.h>

namespace vsrtl {

PortPoint::PortPoint(QGraphicsItem* parent) : GraphicsBase(parent) {}

QRectF PortPoint::boundingRect() const {
#ifdef VSRTL_DEBUG_DRAW
    return QRectF(-WIRE_WIDTH / 2, -WIRE_WIDTH / 2, WIRE_WIDTH / 2, WIRE_WIDTH / 2);
#else
    return QRectF();
#endif
}

void PortPoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) {
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

WirePoint::WirePoint(WireGraphic& parent, QGraphicsItem* sceneParent) : PortPoint(sceneParent), m_parent(parent) {
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);
    m_sceneParent = dynamic_cast<ComponentGraphic*>(sceneParent);
    Q_ASSERT(m_sceneParent != nullptr);
}

QRectF WirePoint::boundingRect() const {
    return shape().boundingRect().adjusted(-WIRE_WIDTH, -WIRE_WIDTH, WIRE_WIDTH, WIRE_WIDTH);
}

/**
 * @brief WirePoint::invalidate
 * Called before marking the point for deletion, ensuring that no calls are made to related wire segments, which may
 * also be in the process of deletion
 */
void WirePoint::invalidate() {
    m_outputWires.clear();
    m_inputWire = nullptr;
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

void WirePoint::removeOutputWire(WireSegment* wire) {
    auto iter = std::find(m_outputWires.begin(), m_outputWires.end(), wire);
    Q_ASSERT(iter != m_outputWires.end());
    m_outputWires.erase(iter);
}

void WirePoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    // Given that WirePoints are graphical children of a component graphic (for them to be fixed on the component),
    // they will still be drawn when the component graphic is not expanded (given that it is still visible). Thus,
    // only paint if the parent componentgraphic is expanded.
    if (!m_sceneParent->isExpanded())
        return;

    painter->save();
    QPen pen = m_parent.getPen();

    QColor fillColor = (option->state & QStyle::State_Selected) ? QColor(Qt::yellow) : pen.color();
    if (option->state & QStyle::State_MouseOver)
        fillColor = fillColor.lighter(125);

    painter->setPen(pen);
    if (option->state & QStyle::State_Selected) {
        pen.setWidth(1);
        painter->drawPath(shape());
    }
    painter->fillPath(shape(), QBrush(fillColor.darker(option->state & QStyle::State_Sunken ? 120 : 100)));

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
    if (isLocked())
        return;

    QMenu menu;
    auto removePointAction = menu.addAction("Remove point");
    connect(removePointAction, &QAction::triggered, [this](bool) { m_parent.removeWirePoint(this); });
    menu.exec(event->screenPos());
}

// --------------------------------------------------------------------------------

WireSegment::WireSegment(WireGraphic* parent) : m_parent(parent), GraphicsBase(parent) {
    setAcceptHoverEvents(true);
}

void WireSegment::setStart(PortPoint* start) {
    if (auto* prevStart = dynamic_cast<WirePoint*>(m_start)) {
        prevStart->removeOutputWire(this);
    }
    m_start = start;
    if (auto* newStart = dynamic_cast<WirePoint*>(start)) {
        newStart->addOutputWire(this);
    }
}

void WireSegment::setEnd(PortPoint* end) {
    if (auto* prevEnd = dynamic_cast<WirePoint*>(m_end)) {
        prevEnd->clearInputWire();
    }
    m_end = end;
    if (auto* newEnd = dynamic_cast<WirePoint*>(end)) {
        newEnd->setInputWire(this);
    }
}

QLineF WireSegment::getLine() const {
    if (m_start == nullptr || m_end == nullptr)
        return QLine();

    QPointF startPos;
    if (auto* p = dynamic_cast<PortGraphic*>(m_start->parentItem())) {
        startPos = mapFromItem(p, p->getOutputPoint());
    } else {
        startPos = mapFromItem(m_start->parentItem(), m_start->pos());
    }

    QPointF endPos;
    if (auto* p = dynamic_cast<PortGraphic*>(m_end->parentItem())) {
        endPos = mapFromItem(p, p->getInputPoint());
    } else {
        endPos = mapFromItem(m_end->parentItem(), m_end->pos());
    }

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

void WireSegment::invalidate() {
    setEnd(nullptr);
    setStart(nullptr);
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
    if (isLocked())
        return;

    // Todo: allow adding points. Should possibly just be done by a click on the wire
    QMenu menu;
    auto createPointAction = menu.addAction("Add wire point");
    connect(createPointAction, &QAction::triggered,
            [this, &event](bool) { m_parent->createWirePointOnSeg(event->scenePos(), this); });
    menu.exec(event->screenPos());
}

void WireSegment::mousePressEvent(QGraphicsSceneMouseEvent* event) {}

// --------------------------------------------------------------------------------

WirePoint* WireGraphic::createWirePoint() {
    // Create new point managed by this graphic.
    auto* topComponent = getPointOwningComponentGraphic();
    Q_ASSERT(topComponent != nullptr);
    auto* point = new WirePoint(*this, topComponent);
    m_points.insert(point);
    return point;
}

void WireGraphic::moveWirePoint(PortPoint* point, const QPointF scenePos) {
    // The parent structure is somewhat nested; a wireGraphic must be a graphical child of the graphics component
    // which this wire is nested inside. To reach this, we do: WireGraphic->parent = PortGraphic PortGraphic->parent
    // = ComponentGraphic ComponentGrahic -> parent = ComponentGraphic (with nested components)
    auto* topComponent = getPointOwningComponentGraphic();
    Q_ASSERT(topComponent != nullptr);
    // Move new point to its creation position
    point->setPos(mapToItem(topComponent, mapFromScene(scenePos)));
}

WireSegment* WireGraphic::createSegment(PortPoint* start, PortPoint* end) {
    auto* newSeg = new WireSegment(this);
    newSeg->setStart(start);
    newSeg->setEnd(end);
    m_wires.insert(newSeg);
    return newSeg;
}

std::pair<WirePoint*, WireSegment*> WireGraphic::createWirePointOnSeg(const QPointF scenePos, WireSegment* onSegment) {
    const auto& endPoint = onSegment->getEnd();

    auto* newPoint = createWirePoint();

    // Set the wire segment of which this point was created, to terminate at the new point
    onSegment->setEnd(newPoint);

    // Create wire segment between new point and previous end point
    auto* newSeg = createSegment(newPoint, endPoint);

    // Move new point to its creation position
    moveWirePoint(newPoint, scenePos);

    // Return a pointer to the newly created wire segment and point
    return {newPoint, newSeg};
}

void WireGraphic::removeWirePoint(WirePoint* pointToRemove) {
    Q_ASSERT(std::find(m_points.begin(), m_points.end(), pointToRemove) != m_points.end());
    auto* wireToRemove = pointToRemove->getInputWire();
    auto* newStartPoint = wireToRemove->getStart();
    Q_ASSERT(newStartPoint != nullptr);
    Q_ASSERT(newStartPoint != pointToRemove);

    // Set all wires previously connecting to the input wire to connect with the new start point
    for (auto& wire : pointToRemove->getOutputWires()) {
        wire->setStart(newStartPoint);
    }

    // Delete the (now defunct) wire between the new start point and the point to be removed
    auto iter = std::find(m_wires.begin(), m_wires.end(), wireToRemove);
    Q_ASSERT(iter != m_wires.end());
    (*iter)->invalidate();
    m_wires.erase(iter);

    // Mark the wire for deletion
    wireToRemove->invalidate();
    wireToRemove->deleteLater();

    // Finally, delete the point
    auto p_iter = std::find(m_points.begin(), m_points.end(), pointToRemove);
    Q_ASSERT(p_iter != m_points.end());
    m_points.erase(p_iter);
    pointToRemove->invalidate();
    pointToRemove->deleteLater();
}

WireGraphic::WireGraphic(PortGraphic* from, const std::vector<PortBase*>& to, QGraphicsItem* parent)
    : m_fromPort(from), m_toPorts(to), GraphicsBase(parent) {}

bool WireGraphic::managesPoint(WirePoint* point) const {
    return std::find(m_points.begin(), m_points.end(), point) != m_points.end();
}

void WireGraphic::mergePoints(WirePoint* base, WirePoint* toMerge) {
    const auto mergeType = canMergePoints(base, toMerge);
    Q_ASSERT(mergeType != MergeType::CannotMerge);

    if (mergeType == MergeType::MergeSourceWithSink) {
        auto* tmp = base;
        base = toMerge;
        toMerge = tmp;
    }

    // Move all output wires of the point to merge, to have their source being the base point
    for (const auto& wire : toMerge->getOutputWires()) {
        wire->setStart(base);
    }

    removeWirePoint(toMerge);
}

void WireGraphic::clearWirePoints() {
    for (auto& p : m_points) {
        removeWirePoint(p);
    }
    update();
}

/**
 * @brief WireGraphic::canMergePoints
 * Points which are adjacent may be merged.
 * Points which both have their input wires connected to the source port may be merged.
 */
WireGraphic::MergeType WireGraphic::canMergePoints(WirePoint* base, WirePoint* toMerge) const {
    if (!managesPoint(base) || !managesPoint(toMerge))
        return MergeType::CannotMerge;

    auto* baseptr = static_cast<PortPoint*>(base);

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
 * @brief WireGraphic::getPointOwningComponent
 * When creating new WireGraphic points, they must be the graphical children of some higher-level component, such
 * that they move with the component. This function locates, based on the type of port which this WireGraphic is created
 * with, the respective parent component to create a WirePoint as a child of.
 * Returns the component which will be the graphical parent of points created and managed by this wireGraphic
 */
ComponentGraphic* WireGraphic::getPointOwningComponentGraphic() const {
    // Initial hierarchy: WireGraphic -> portGraphic -> ComponentGraphic
    auto* portParent = dynamic_cast<PortGraphic*>(parentItem());
    auto* componentParent = dynamic_cast<ComponentGraphic*>(portParent->parentItem());
    Q_ASSERT(portParent && componentParent);
    if (componentParent->hasSubcomponents()) {
        // If this wire graphic is the graphic of a port which is on the I/O boundary of a component with subcomponents,
        // return the parent component
        return componentParent;
    } else {
        // Else, we are a subcomponent within a component
        return componentParent->getParent();
    }
}

Component* WireGraphic::getPointOwningComponent() const {
    return getPointOwningComponentGraphic()->getComponent();
}

void WireGraphic::createRectilinearSegments(PortPoint* start, PortPoint* end) {
    // 1. Create the initial segment between the two terminating points
    auto* seg = createSegment(start, end);

    // 2. Determine intermediate rectilinear points
    auto line = seg->getLine().toLine();
    if (line.y1() == line.y2() && line.x1() < line.x2()) {
        // Nothing to do, already linear and going from left to right
        return;
    }

    QPoint intermediate1, intermediate2;
    bool createTwoPoints = true;

    if (line.x1() < line.x2()) {
        // left to right wire, route directly
        intermediate1 = {line.x1() + line.dx() / 2, line.y1()};
        intermediate2 = {line.x1() + line.dx() / 2, line.y2()};
    } else if (dynamic_cast<PortGraphic*>(start->parentItem()) && dynamic_cast<PortGraphic*>(end->parentItem())) {
        // Routing between two components
        // Route underneath the source and destination components
        auto* compSource = dynamic_cast<ComponentGraphic*>(start->parentItem()->parentItem());
        auto* compDst = dynamic_cast<ComponentGraphic*>(end->parentItem()->parentItem());

        QRectF sourceRect = mapRectFromItem(compSource, compSource->boundingRect());
        QRectF destRect = mapRectFromItem(compDst, compDst->boundingRect());

        int y = sourceRect.bottom() < destRect.bottom() ? sourceRect.bottom() : destRect.bottom();
        intermediate1 = QPoint{line.x1(), y};
        intermediate2 = QPoint{line.x2(), y};
    } else {
        // Routing between wire points, just create a single intermediate point
        createTwoPoints = false;
        if (line.x1() < line.x2()) {
            intermediate1 = QPoint{line.x1(), line.y2()};
        } else {
            intermediate1 = QPoint{line.x2(), line.y1()};
        }
    }
    // 3. Create points on wire segments
    auto pointAndSeg = createWirePointOnSeg(mapToScene(intermediate1), seg);
    if (createTwoPoints)
        pointAndSeg = createWirePointOnSeg(mapToScene(intermediate2), pointAndSeg.second);
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

    // Make the wire destination ports aware of this WireGraphic, and create wire segments between all source and sink
    // ports.
    for (const auto& sink : m_toGraphicPorts) {
        sink->setInputWire(this);
        // Create a rectilinear segment between the the closest point managed by this wire and the sink destination
        std::pair<qreal, PortPoint*> fromPoint;
        const QPointF sinkPos = sink->getPointGraphic()->scenePos();
        fromPoint.first = (sinkPos - m_fromPort->getPointGraphic()->scenePos()).manhattanLength();
        fromPoint.second = m_fromPort->getPointGraphic();
        for (const auto& p : m_points) {
            const qreal len = (sinkPos - p->scenePos()).manhattanLength();
            if (len < fromPoint.first) {
                fromPoint = {len, p};
            }
        }
        createRectilinearSegments(fromPoint.second, sink->getPointGraphic());
    }

    GraphicsBase::postSceneConstructionInitialize1();
}

const QPen& WireGraphic::getPen() {
    // propagate the source port pen to callers
    return m_fromPort->getPen();
}

}  // namespace vsrtl
