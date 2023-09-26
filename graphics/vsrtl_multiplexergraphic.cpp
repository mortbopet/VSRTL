#include "vsrtl_multiplexergraphic.h"
#include "vsrtl_portgraphic.h"

#include <QPainter>

namespace vsrtl {

MultiplexerGraphic::MultiplexerGraphic(SimComponent *c,
                                       ComponentGraphic *parent)
    : ComponentGraphic(c, parent) {
  // Make changes in the select signal trigger a redraw of the multiplexer (and
  // its input signal markings)
  wrapSimSignal(getSelect()->changed);
}

SimPort *MultiplexerGraphic::getSelect() {
  // Simulator component must have set the select port to special port 0
  return m_component->getSpecialPort(GFX_MUX_SELECT);
}

void MultiplexerGraphic::paintOverlay(QPainter *painter,
                                      const QStyleOptionGraphicsItem *,
                                      QWidget *) {
  // Mark input IO with circles, highlight selected input with green
  painter->save();

  const auto inputPorts = m_component->getPorts<SimPort::PortType::in>();
  const auto *select = getSelect();
  const unsigned int index = select->uValue();
  Q_ASSERT(static_cast<long>(index) < m_inputPorts.size());

  for (const auto &ip : std::as_const(m_inputPorts)) {
    const auto *p_base = ip->getPort();
    if (p_base != select) {
      if (p_base == inputPorts[index]) {
        painter->setBrush(Qt::green);
      } else {
        painter->setBrush(Qt::white);
      }

      paintIndicator(painter, ip,
                     p_base == inputPorts[index] ? Qt::green : Qt::white);
    }
  }
  painter->restore();
}

} // namespace vsrtl
