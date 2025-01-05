#include "vsrtl_wiregraphic.h"
#include "vsrtl_componentgraphic.h"
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

namespace {

unsigned int visibleOutputWires(PortPoint *p) {
  unsigned int visibleWires = 0;
  for (const auto &w : p->getOutputWires()) {
    visibleWires += w->isVisible() && w->isDrawn() ? 1 : 0;
  }
  return visibleWires;
}
} // namespace

std::vector<std::string> getPortParentNameSeq(SimPort *p) {
  std::vector<std::string> seq;
  seq.push_back(p->getName());
  seq.push_back(p->getParent()->getName());
  return seq;
}

PortPoint::PortPoint(QGraphicsItem *parent) : GraphicsBaseItem(parent) {
  setAcceptHoverEvents(false);
  m_portParent = dynamic_cast<PortGraphic *>(parent);

  m_shape = QPainterPath();
  m_shape.addEllipse({0, 0}, WIRE_WIDTH * 1.5, WIRE_WIDTH * 1.5);
  m_shape.setFillRule(Qt::WindingFill);
  m_br = m_shape.boundingRect().adjusted(-WIRE_WIDTH, -WIRE_WIDTH, WIRE_WIDTH,
                                         WIRE_WIDTH);
}

void PortPoint::modulePositionHasChanged() { portPosChanged(); }

void PortPoint::portPosChanged() {
  // Propagate a redraw call to all segments connecting to this
  if (m_inputWire) {
    m_inputWire->geometryModified();
  }
  for (const auto &wire : m_outputWires) {
    wire->geometryModified();
  }
}

QVariant PortPoint::itemChange(GraphicsItemChange change,
                               const QVariant &value) {
  if (change == QGraphicsItem::ItemPositionHasChanged) {
    portPosChanged();
  }

  return GraphicsBaseItem::itemChange(change, value);
}

QRectF PortPoint::boundingRect() const { return m_br; }
QPainterPath PortPoint::shape() const { return m_shape; }

void PortPoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                      QWidget *) {
  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());
  if (lod < 0.35)
    return;

  // Do not draw point when only a single output wire exists, and we are not
  // currently interacting with the point
  if (visibleOutputWires(this) <= 1 && m_draggedOnThis == nullptr &&
      !(option->state & (QStyle::State_Selected | QStyle::State_MouseOver))) {
    return;
  }

  painter->save();
  QPen pen = getPen();

  QColor fillColor = (option->state & QStyle::State_Selected)
                         ? QColorConstants::Yellow
                         : pen.color();
  if (option->state & QStyle::State_MouseOver)
    fillColor = fillColor.lighter(125);

  painter->setPen(pen);
  if (option->state & QStyle::State_Selected) {
    pen.setWidth(1);
    painter->drawPath(shape());
  }
  painter->fillPath(shape(),
                    QBrush(fillColor.darker(
                        (option->state & QStyle::State_Sunken) ? 120 : 100)));

  if (m_draggedOnThis != nullptr) {
    pen.setColor(Qt::red);
    painter->setPen(pen);
    painter->setBrush(Qt::transparent);
    painter->drawRect(shape().boundingRect().adjusted(
        -WIRE_WIDTH / 2, -WIRE_WIDTH / 2, WIRE_WIDTH / 2, WIRE_WIDTH / 2));
  }
  painter->restore();
}

const QPen &PortPoint::getPen() {
  // If getPen is called on a PortPoint, it is required that the parent item is
  // a PortGraphic. PortGraphics are the only classes which should instatiate a
  // PortPoint.
  Q_ASSERT(m_portParent);
  return m_portParent->getPen();
}

void PortPoint::removeOutputWire(WireSegment *wire) {
  auto iter = std::find(m_outputWires.begin(), m_outputWires.end(), wire);
  Q_ASSERT(iter != m_outputWires.end());
  m_outputWires.erase(iter);
}

bool WirePoint::canMergeWith(WirePoint *point) {
  return m_parent->canMergePoints(this, point) !=
         WireGraphic::MergeType::CannotMerge;
}

void WirePoint::pointDrop(WirePoint *point) {
  if (!canMergeWith(point))
    return;

  m_parent->mergePoints(this, point);
}

void WirePoint::pointDragEnter(WirePoint *point) {
  if (m_parent->canMergePoints(this, point) !=
      WireGraphic::MergeType::CannotMerge) {
    m_draggedOnThis = point;
    update();
  }
}

void WirePoint::pointDragLeave(WirePoint *) {
  m_draggedOnThis = nullptr;
  update();
}

WirePoint::WirePoint(WireGraphic *parent)
    : PortPoint(parent), m_parent(parent) {
  setFlag(ItemIsSelectable, true);
  setAcceptHoverEvents(true);
  setMoveable();
}

const QPen &WirePoint::getPen() { return m_parent->getPen(); }

QVariant WirePoint::itemChange(GraphicsItemChange change,
                               const QVariant &value) {
  if (change == QGraphicsItem::ItemPositionChange) {
    // Disallow WirePoint moving when scene is locked
    if (isLocked() && !m_parent->isSerializing())
      return pos();

    // Snap to grid
    QPointF newPos = value.toPointF();
    qreal x = round(newPos.x() / GRID_SIZE) * GRID_SIZE;
    qreal y = round(newPos.y() / GRID_SIZE) * GRID_SIZE;
    return QPointF(x, y);
  }

  return PortPoint::itemChange(change, value);
}

void WirePoint::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
  if (isLocked())
    return;

  QMenu menu;
  auto removePointAction = menu.addAction("Remove point");
  connect(removePointAction, &QAction::triggered,
          [this](bool) { m_parent->removeWirePoint(this); });
  menu.exec(event->screenPos());
}

// --------------------------------------------------------------------------------

WireSegment::WireSegment(WireGraphic *parent)
    : GraphicsBaseItem(parent), m_parent(parent) {
  setAcceptHoverEvents(true);
  setFlag(ItemIsSelectable);
}

void WireSegment::setStart(PortPoint *start) {
  if (m_start)
    m_start->removeOutputWire(this);
  m_start = start;
  if (m_start)
    m_start->addOutputWire(this);

  geometryModified();
}

void WireSegment::setEnd(PortPoint *end) {
  if (m_end)
    m_end->clearInputWire();
  m_end = end;
  if (m_end)
    m_end->setInputWire(this);

  geometryModified();
}

QLineF WireSegment::getLine() const {
  if (m_start == nullptr || m_end == nullptr)
    return QLine();

  QPointF startPos;
  if (auto *p = dynamic_cast<PortGraphic *>(m_start->parentItem())) {
    startPos = mapFromItem(p, p->getOutputPoint());
  } else {
    startPos = mapFromItem(m_start->parentItem(), m_start->pos());
  }

  QPointF endPos;
  if (auto *p = dynamic_cast<PortGraphic *>(m_end->parentItem())) {
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
  setStart(nullptr);
  setEnd(nullptr);
}

void WireSegment::geometryModified() {
  QPointF p1_mod, p2_mod;
  prepareGeometryChange();
  if (!isValid()) {
    goto geometryModified_invalidate;
  }

  m_cachedLine = getLine();
  if (m_cachedLine.p1() == m_cachedLine.p2()) {
    goto geometryModified_invalidate;
  }

  // The shape of the line is defined as a box of WIRE_WIDTH around the entire
  // line between the two end points. Except for WIRE_WIDTH/2 at the edges of
  // the line, wherein we would like to be able to click on a potential
  // WirePoint.
  m_cachedShape = QPainterPath();
  p1_mod = m_cachedLine.pointAt(WIRE_WIDTH / m_cachedLine.length());
  p2_mod = m_cachedLine.pointAt((m_cachedLine.length() - WIRE_WIDTH) /
                                m_cachedLine.length());
  m_cachedShape.addPolygon(expandLine(QLineF(p1_mod, p2_mod), WIRE_WIDTH));

  // The bounding rect is an equivalently expanded box around the current line,
  // but with full length.
  m_cachedBoundingRect = expandLine(m_cachedLine, WIRE_WIDTH).boundingRect();
  return;

geometryModified_invalidate:
  m_cachedShape = QPainterPath();
  m_cachedLine = QLine();
  m_cachedBoundingRect = QRectF();
  return;
}

QPainterPath WireSegment::shape() const {
  if (!isValid())
    return QPainterPath();

  return m_cachedShape;
}

QRectF WireSegment::boundingRect() const {
  if (!isValid())
    return QRectF();

  return m_cachedBoundingRect;
}

void WireSegment::hoverMoveEvent(QGraphicsSceneHoverEvent *) {
  setToolTip(m_parent->getFromPort()->getTooltipString());
}

bool WireSegment::isValid() const {
  return m_start != nullptr && m_end != nullptr;
}

bool WireSegment::isDrawn() const {
  return isValid() && m_start->isVisible() && m_end->isVisible();
}

void WireSegment::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  if (!isDrawn())
    return;

  painter->save();
  painter->setPen(m_parent->getPen());
  painter->drawLine(m_cachedLine);
  painter->restore();
#ifdef VSRTL_DEBUG_DRAW
  DRAW_BOUNDING_RECT(painter)
#endif
}

void WireSegment::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
  if (isLocked())
    return;

  // Todo: allow adding points. Should possibly just be done by a click on the
  // wire
  QMenu menu;
  auto createPointAction = menu.addAction("Add wire point");
  connect(createPointAction, &QAction::triggered, [this, &event](bool) {
    m_parent->createWirePointOnSeg(event->scenePos(), this);
  });
  menu.exec(event->screenPos());
}

QVariant WireSegment::itemChange(GraphicsItemChange change,
                                 const QVariant &value) {
  if (change == QGraphicsItem::ItemSelectedChange) {
    m_parent->getFromPort()->itemChange(change, value);
  }

  return GraphicsBaseItem::itemChange(change, value);
}

// --------------------------------------------------------------------------------

WirePoint *WireGraphic::createWirePoint() {
  // Create new point managed by this graphic.
  auto *point = new WirePoint(this);
  m_points.insert(point);
  return point;
}

void WireGraphic::moveWirePoint(PortPoint *point, const QPointF scenePos) {
  // Move new point to its creation position
  point->setPos(mapToItem(this, mapFromScene(scenePos)));
}

WireSegment *WireGraphic::createSegment(PortPoint *start, PortPoint *end) {
  auto *newSeg = new WireSegment(this);
  newSeg->setStart(start);
  newSeg->setEnd(end);
  m_wires.insert(newSeg);
  return newSeg;
}

std::pair<WirePoint *, WireSegment *>
WireGraphic::createWirePointOnSeg(const QPointF scenePos,
                                  WireSegment *onSegment) {
  const auto &endPoint = onSegment->getEnd();

  auto *newPoint = createWirePoint();

  // Set the wire segment of which this point was created, to terminate at the
  // new point
  onSegment->setEnd(newPoint);

  // Create wire segment between new point and previous end point
  auto *newSeg = createSegment(newPoint, endPoint);

  // Move new point to its creation position
  moveWirePoint(newPoint, scenePos);

  // Return a pointer to the newly created wire segment and point
  return {newPoint, newSeg};
}

void WireGraphic::removeWirePoint(WirePoint *pointToRemove) {
  prepareGeometryChange();
  Q_ASSERT(std::find(m_points.begin(), m_points.end(), pointToRemove) !=
           m_points.end());
  auto *wireToRemove = pointToRemove->getInputWire();
  auto *newStartPoint = wireToRemove->getStart();
  Q_ASSERT(newStartPoint != nullptr);
  Q_ASSERT(newStartPoint != pointToRemove);

  // Set all wires previously connecting to the input wire to connect with the
  // new start point
  for (auto &wire : pointToRemove->getOutputWires()) {
    Q_ASSERT(wire->getStart() == pointToRemove);
    wire->setStart(newStartPoint);
  }
  Q_ASSERT(pointToRemove->getOutputWires().size() == 0);

  // Delete the (now defunct) wire between the new start point and the point to
  // be removed
  auto iter = std::find(m_wires.begin(), m_wires.end(), wireToRemove);
  Q_ASSERT(iter != m_wires.end());
  m_wires.erase(iter);
  wireToRemove->invalidate();
  delete wireToRemove;

  // Finally, delete the point. At this point, no wires should be referencing
  // the point
  Q_ASSERT(pointToRemove->getInputWire() == nullptr &&
           pointToRemove->getOutputWires().empty());
  auto p_iter = std::find(m_points.begin(), m_points.end(), pointToRemove);
  Q_ASSERT(p_iter != m_points.end());
  m_points.erase(p_iter);
  pointToRemove->deleteLater();
}

WireGraphic::WireGraphic(ComponentGraphic *parent, PortGraphic *from,
                         const std::vector<SimPort *> &to, WireType type)
    : GraphicsBaseItem(parent), m_parent(parent), m_fromPort(from),
      m_toPorts(to), m_type(type) {
  m_parent->registerWire(this);
  setFlag(QGraphicsItem::ItemHasNoContents, true);
}

bool WireGraphic::managesPoint(WirePoint *point) const {
  return std::find(m_points.begin(), m_points.end(), point) != m_points.end();
}

void WireGraphic::mergePoints(WirePoint *base, WirePoint *toMerge) {
  const auto mergeType = canMergePoints(base, toMerge);
  Q_ASSERT(mergeType != MergeType::CannotMerge);

  if (mergeType == MergeType::MergeSourceWithSink) {
    auto *tmp = base;
    base = toMerge;
    toMerge = tmp;
  }

  // Move all output wires of the point to merge, to have their source being the
  // base point
  for (const auto &wire : toMerge->getOutputWires()) {
    wire->setStart(base);
  }

  removeWirePoint(toMerge);
}

void WireGraphic::clearWirePoints() {
  // removeWirePoint modifies m_points. Iterate over a copy.
  auto pointsCopy = m_points;
  for (auto &p : pointsCopy) {
    removeWirePoint(p);
  }
}

void WireGraphic::clearWires() {
  for (const auto &w : m_wires) {
    w->invalidate();
    delete w;
  }
  m_wires.clear();
}

/**
 * @brief WireGraphic::canMergePoints
 * Points which are adjacent may be merged.
 * Points which both have their input wires connected to the source port may be
 * merged.
 */
WireGraphic::MergeType WireGraphic::canMergePoints(WirePoint *base,
                                                   WirePoint *toMerge) const {
  if (!managesPoint(base) || !managesPoint(toMerge))
    return MergeType::CannotMerge;

  auto *baseptr = static_cast<PortPoint *>(base);

  // is toMerge fed by base?
  if (baseptr == toMerge->getInputWire()->getStart())
    return MergeType::MergeSinkWithSource;

  // does toMerge feed into base?
  for (const auto &wire : toMerge->getOutputWires()) {
    if (wire->getEnd() == baseptr)
      return MergeType::MergeSourceWithSink;
  }

  // is toMerge and base fed by the same source port?
  if (base->getInputWire()->getStart() == toMerge->getInputWire()->getStart())
    return MergeType::MergeParallelSinks;

  return MergeType::CannotMerge;
}

void WireGraphic::createRectilinearSegments(PortPoint *start, PortPoint *end) {
  // 1. Create the initial segment between the two terminating points
  auto *seg = createSegment(start, end);

  // 2. Determine intermediate rectilinear points
  auto line = seg->getLine().toLine();
  if ((line.y1() == line.y2() && line.x1() <= line.x2()) ||
      line.x1() == line.x2()) {
    // Nothing to do, already linear and going from left to right
    return;
  }

  QPoint intermediate1, intermediate2;
  bool createTwoPoints = true;

  if (line.x1() < line.x2()) {
    // left to right wire, route directly
    intermediate1 = {line.x1() + line.dx() / 2, line.y1()};
    intermediate2 = {line.x1() + line.dx() / 2, line.y2()};
  } else if (dynamic_cast<PortGraphic *>(start->parentItem()) &&
             dynamic_cast<PortGraphic *>(end->parentItem())) {
    // Routing between two components
    // Route underneath the source and destination components
    auto *compSource =
        dynamic_cast<ComponentGraphic *>(start->parentItem()->parentItem());
    auto *compDst =
        dynamic_cast<ComponentGraphic *>(end->parentItem()->parentItem());

    QRectF sourceRect = mapRectFromItem(compSource, compSource->boundingRect());
    QRectF destRect = mapRectFromItem(compDst, compDst->boundingRect());

    const int y = static_cast<int>(sourceRect.bottom() < destRect.bottom()
                                       ? sourceRect.bottom()
                                       : destRect.bottom());
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
    createWirePointOnSeg(mapToScene(intermediate2), pointAndSeg.second);
}

/**
 * @brief WireGraphic::setWiresVisibleToPort
 * Used when a component graphic toggles its visibility. In this case, wires and
 * points which traverse towards at an input port to that component should be
 * hidden
 * @param p port
 * @param visible
 */
void WireGraphic::setWiresVisibleToPort(const PortPoint *p, bool visible) {
  /// Find segment which terminates in @param p
  auto iter =
      std::find_if(m_wires.begin(), m_wires.end(),
                   [p](WireSegment *wire) { return wire->getEnd() == p; });
  Q_ASSERT(iter != m_wires.end());

  WireSegment *seg = *iter;
  PortPoint *start = seg->getStart();
  while (seg->isVisible() ^ visible) {
    seg->setVisible(visible);
    // Recurse?
    bool recurse = false;
    if (visible) {
      /// All wire points between this WireGraphic and the root port should be
      /// enabled.
      recurse = dynamic_cast<WirePoint *>(start);
    } else {
      /// Traverse the sequence of wires from @param p towards @this source
      /// port, until we encounter a WirePoint which has more than 0 >visible<
      /// wire enabled
      recurse =
          dynamic_cast<WirePoint *>(start) && (visibleOutputWires(start) == 0);
    }
    if (recurse) {
      start->setVisible(visible);
      seg = start->getInputWire();
      start = seg->getStart();
    } else {
      break;
    }
  }
}

SimComponent *WireGraphic::getParentComponent() const {
  return m_parent->getComponent();
}

/**
 * @brief WireGraphic::postSceneConstructionInitialize1
 * With all ports and components created during circuit construction, wires may
 * now register themselves with their attached input- and output ports
 */
void WireGraphic::postSceneConstructionInitialize1() {
  std::function<void(SimPort *)> addGraphicToPort = [&](SimPort *portPtr) {
    if (auto *portGraphic = portPtr->getGraphic<PortGraphic>()) {
      m_toGraphicPorts.push_back(portGraphic);
    }
    if (portPtr->type() == vsrtl::SimPort::PortType::signal) {
      for (auto toPort : portPtr->getOutputPorts()) {
        // Signals are only relevant for the underlying circuit - traverse to
        // the output ports of the signal
        addGraphicToPort(toPort);
      }
    }
  };

  for (const auto &toPort : m_toPorts) {
    addGraphicToPort(toPort);
  }

  // Make the wire destination ports aware of this WireGraphic, and create wire
  // segments between all source and sink ports.
  for (const auto &sink : m_toGraphicPorts) {
    sink->setInputWire(this);
    // Create a rectilinear segment between the the closest point managed by
    // this wire and the sink destination
    std::pair<qreal, PortPoint *> fromPoint;
    const QPointF sinkPos =
        sink->getPortPoint(vsrtl::SimPort::PortType::in)->scenePos();
    fromPoint.first =
        (sinkPos -
         m_fromPort->getPortPoint(vsrtl::SimPort::PortType::out)->scenePos())
            .manhattanLength();
    fromPoint.second = m_fromPort->getPortPoint(vsrtl::SimPort::PortType::out);
    for (const auto &p : m_points) {
      const qreal len = (sinkPos - p->scenePos()).manhattanLength();
      if (len < fromPoint.first) {
        fromPoint = {len, p};
      }
    }
    createRectilinearSegments(fromPoint.second,
                              sink->getPortPoint(vsrtl::SimPort::PortType::in));
  }

  GraphicsBaseItem::postSceneConstructionInitialize1();
}

void WireGraphic::postSerializeInit() {
  if (m_fromPort) {
    m_fromPort->setUserVisible(!m_fromPort->userHidden());
  }

  for (const auto &p : m_toGraphicPorts) {
    p->setUserVisible(!p->userHidden());
  }

  // Geometry updating is disabled whilst serializing. Make all wires update
  // geometry after serialization has completed.
  for (const auto &wire : m_wires) {
    wire->geometryModified();
  }
}

const QPen &WireGraphic::getPen() {
  // propagate the source port pen to callers
  return m_fromPort->getPen();
}

} // namespace vsrtl
