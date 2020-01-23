#include "vsrtl_multiplexergraphic.h"
#include "vsrtl_portgraphic.h"

#include <QPainter>

namespace vsrtl {

MultiplexerGraphic::MultiplexerGraphic(SimComponent& c, ComponentGraphic* parent) : ComponentGraphic(c, parent) {
    // Make changes in the select signal trigger a redraw of the multiplexer (and its input signal markings)
    getSelect()->changed.Connect(this, &ComponentGraphic::updateSlot);
}

SimPort* MultiplexerGraphic::getSelect() {
    // Simulator component must have set the select port to special port 0
    return m_component.getSpecialPort("select");
}

void MultiplexerGraphic::paintOverlay(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    // Mark input IO with circles, highlight selected input with green
    painter->save();

    const auto inputPorts = m_component.getPorts<SimPort::Direction::in>();
    const auto* select = getSelect();
    const unsigned int index = select->uValue();
    Q_ASSERT(index < m_inputPorts.size());

    for (const auto& ip : m_inputPorts) {
        const auto* p_base = ip->getPort();
        if (p_base != select) {
            if (p_base == inputPorts[index]) {
                painter->setBrush(Qt::green);
            } else {
                painter->setBrush(Qt::white);
            }

            constexpr qreal dotSize = 13;
            QRectF chordRect(-dotSize / 2, -dotSize / 2, dotSize, dotSize);
            chordRect.translate(mapFromItem(ip, ip->getOutputPoint()));
            QPen pen = painter->pen();
            pen.setWidth(WIRE_WIDTH - 1);
            painter->setPen(pen);
            painter->drawChord(chordRect, -90 * 16, 180 * 16);
        }
    }
    painter->restore();
}

}  // namespace vsrtl
