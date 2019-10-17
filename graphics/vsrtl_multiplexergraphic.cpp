#include "vsrtl_multiplexergraphic.h"
#include "vsrtl_portgraphic.h"

#include <QPainter>

namespace vsrtl {

MultiplexerGraphic::MultiplexerGraphic(MultiplexerBase& c) : ComponentGraphic(c), m_multiplexer(c) {
    // Make changes in the select signal trigger a redraw of the multiplexer (and its input signal markings)
    c.getSelect()->changed.Connect(this, &ComponentGraphic::updateSlot);
}

void MultiplexerGraphic::paintOverlay(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    // Mark input IO with circles, highlight selected input with green
    painter->save();
    for (const auto& ip : m_inputPorts) {
        const auto* p_base = ip->getPort();
        if (p_base != m_multiplexer.getSelect()) {
            const unsigned int index = m_multiplexer.getSelect()->uValue();
            Q_ASSERT(index < m_multiplexer.getIns().size());
            if (p_base == m_multiplexer.getIns()[index]) {
                painter->setBrush(Qt::green);
            } else {
                painter->setBrush(Qt::white);
            }

            constexpr qreal dotSize = 14;
            QRectF chordRect(-dotSize / 2, -dotSize / 2, dotSize, dotSize);
            chordRect.translate(mapFromItem(ip, ip->getOutputPoint()));
            QPen pen = painter->pen();
            pen.setWidth(WIRE_WIDTH / 2);
            painter->setPen(pen);
            painter->drawChord(chordRect, -90 * 16, 180 * 16);
        }
    }
    painter->restore();
}

}  // namespace vsrtl
