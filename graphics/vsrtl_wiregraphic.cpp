#include "vsrtl_wiregraphic.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_port.h"
#include "vsrtl_portgraphic.h"

#include "vsrtl_graphics_defines.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPolygon>

namespace vsrtl {

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

void WireGraphic::setNet(const pr::Net& net) {
    m_net = net;
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
    for (const auto& port : m_toGraphicPorts) {
        port->setInputWire(this);
    }

    GraphicsBase::postSceneConstructionInitialize1();
}

const QPen& WireGraphic::getPen() {
    // propagate the source port pen to callers
    return m_fromPort->getPen();
}

QPointF drawNextPoint(QPainter* painter, QPointF from, QPointF intermediate) {
    painter->drawLine(from, intermediate);
    return intermediate;
}

void WireGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    painter->setPen(getPen());

    if (m_net.routes.size() > 0) {
        auto inputPortParent = dynamic_cast<ComponentGraphic*>(m_fromPort->parentItem());
        auto subcomponentParent = dynamic_cast<ComponentGraphic*>(inputPortParent->parentItem());
        for (const auto& route : m_net.routes) {
            Q_ASSERT(route.start.port == m_fromPort);
            // Find destination point - m_net.nodes contains #source node + #destination nodes, m_net.routes contains
            // #destination_nodes routes
            const auto& toPort = route.end.port;
            const QPointF end = mapFromItem(toPort, toPort->getInputPoint());
            QPointF intermediate;
            QPointF from = mapFromItem(route.start.port, route.start.port->getOutputPoint());
            for (int i = 0; i < route.path.size(); i++) {
                bool drawIntermediate = true;
                // The following two statements looks like duplicate code. However, it is important that the code path
                // for each conditional segment is hit when route.path.size() == 1
                if (i == 0) {
                    intermediate = mapFromItem(subcomponentParent, gridToScene(route.path[i]->r).center());
                    intermediate.setY(from.y());
                    from = drawNextPoint(painter, from, intermediate);
                    drawIntermediate = false;
                }
                if (i == (route.path.size() - 1)) {
                    intermediate = mapFromItem(subcomponentParent, gridToScene(route.path[i]->r).center());
                    intermediate.setY(end.y());
                    from = drawNextPoint(painter, from, intermediate);
                    drawIntermediate = false;
                }

                if (drawIntermediate) {
                    from = drawNextPoint(painter, from,
                                         mapFromItem(subcomponentParent, gridToScene(route.path[i]->r).center()));
                }
            }
            painter->drawLine(from, end);
        }
    }
    painter->restore();
}
}  // namespace vsrtl
