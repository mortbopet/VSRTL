#include "vsrtl_wiregraphic.h"
#include "vsrtl_port.h"
#include "vsrtl_portgraphic.h"

#include <QGraphicsScene>
#include <QPainter>
#include <QPolygon>

namespace vsrtl {

WireGraphic::WireGraphic(PortGraphic* from, const std::vector<PortBase*>& to, QGraphicsItem* parent)
    : m_fromPort(from), m_toPorts(to) {
    setParentItem(parent);
}

QRectF WireGraphic::boundingRect() const {
    QPolygonF p;
    p.append(mapFromItem(m_fromPort, m_fromPort->getConnectionPoint()));
    for (const auto& to : m_toGraphicPorts) {
        p.append(mapFromItem(to, to->getConnectionPoint()));
    }
    return p.boundingRect();
}

void WireGraphic::postSceneConstructionInitialize() {
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

    GraphicsBase::postSceneConstructionInitialize();
}
void WireGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    for (const auto& toPort : m_toGraphicPorts) {
        painter->drawLine(mapFromItem(m_fromPort, m_fromPort->getConnectionPoint()),
                          mapFromItem(toPort, toPort->getConnectionPoint()));
    }

    /*
    // draw bounding rect
    painter->save();
    painter->setPen(Qt::green);
    const auto br = boundingRect();
    painter->drawRect(br);
    painter->restore();
    */
}
}  // namespace vsrtl
