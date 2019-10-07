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

namespace vsrtl {

PointGraphic::PointGraphic(QGraphicsItem* parent) {
    setParentItem(parent);
}

WirePoint::WirePoint(WireSegment& parent) : PointGraphic(&parent), m_parent(parent) {
    setParentItem(&parent);
}

QRectF WirePoint::boundingRect() const {
    return QRectF(-WIRE_WIDTH / 2, -WIRE_WIDTH / 2, WIRE_WIDTH / 2, WIRE_WIDTH / 2);
}

void WirePoint::paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) {
    painter->drawPoint(QPoint(0, 0));
}

void WirePoint::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {}

// --------------------------------------------------------------------------------

WireSegment::WireSegment(WireGraphic& parent) : m_parent(parent) {
    setParentItem(&parent);
}

QLineF WireSegment::getLine() const {
    if (m_start == nullptr || m_end == nullptr)
        return QLine();

    const auto& startPos = mapFromItem(m_start, m_start->pos());
    const auto& endPos = mapFromItem(m_end, m_end->pos());
    return QLineF(startPos, endPos);
}

QPainterPath WireSegment::shape() const {
    // @todo: expand the shape a bit out from the wire, for easier clicking

    if (m_start == nullptr || m_end == nullptr)
        return QPainterPath();

    const auto line = getLine();
    QPainterPath path(line.p1());
    path.lineTo(line.p2());
    return path;
}

QRectF WireSegment::boundingRect() const {
    if (m_start == nullptr || m_end == nullptr)
        return QRectF();

    auto br = shape().boundingRect();
    br.adjust(-WIRE_WIDTH, -WIRE_WIDTH, WIRE_WIDTH, WIRE_WIDTH);
    return br;
}

void WireSegment::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    if (!m_start->isVisible() || !m_end->isVisible())
        return;

    painter->save();
    painter->setPen(m_parent.getPen());
    painter->drawLine(getLine());
    painter->restore();
#ifdef VSRTL_DEBUG_DRAW
    DRAW_BOUNDING_RECT(painter)
#endif
}

void WireSegment::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    // Todo: allow adding points. Should possibly just be done by a click on the wire
}

// --------------------------------------------------------------------------------

WireGraphic::WireGraphic(PortGraphic* from, const std::vector<Port*>& to, QGraphicsItem* parent)
    : m_fromPort(from), m_toPorts(to) {
    setParentItem(parent);
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
    // To ensure that input ports are redrawn when this wire changes (ie. gets selected), we overlap the bounding rect
    // of this item onto both of its ports, ensuring redraws
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
        auto seg = std::make_unique<WireSegment>(*this);
        seg->setStart(m_fromPort->getPointGraphic());
        seg->setEnd(sink->getPointGraphic());
        m_segments.push_back(std::move(seg));
    }

    GraphicsBase::postSceneConstructionInitialize1();
}

const QPen& WireGraphic::getPen() {
    // propagate the source port pen to callers
    return m_fromPort->getPen();
}

}  // namespace vsrtl
