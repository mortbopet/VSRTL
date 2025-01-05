#include "vsrtl_valuelabel.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_portgraphic.h"

#include <QAction>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

namespace vsrtl {

ValueLabel::ValueLabel(QGraphicsItem *parent,
                       const std::shared_ptr<Radix> &radix,
                       const PortGraphic *port,
                       std::shared_ptr<QAction> visibilityAction)
    : Label(parent, "", visibilityAction, 10), m_radix(radix), m_port(port) {
  setFlag(ItemIsSelectable, true);
  setAcceptHoverEvents(true);
  updateText();

  m_showLineToPortAction =
      std::make_unique<QAction>("Always show line to port");
  m_showLineToPortAction->setCheckable(true);
  m_showLineToPortAction->setChecked(false);
  QObject::connect(m_showLineToPortAction.get(), &QAction::triggered,
                   [this] { createLineToPort(); });
}

void ValueLabel::paint(QPainter *painter,
                       const QStyleOptionGraphicsItem *option, QWidget *w) {
  // Paint a label box behind the text
  painter->save();
  if (!m_port->getPort()->isConstant()) {
    const bool darkmode = static_cast<VSRTLScene *>(scene())->darkmode();

    QRectF textRect = shape().boundingRect();
    painter->fillRect(textRect, darkmode ? QColor{0x45, 0x45, 0x45}
                                         : QColorConstants::White);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(Qt::black, 1));
    painter->drawRect(textRect);
  }
  painter->restore();

  Label::paint(painter, option, w);
}

void ValueLabel::updateLine() {
  if (m_lineToPort) {
    m_lineToPort->setLine(lineToPort());
  }
}

QLineF ValueLabel::lineToPort() const {
  const QRectF textRect = shape().boundingRect();
  QLineF shortestLine;
  qreal minLength = qInf();
  for (const auto &corner : {textRect.topLeft(), textRect.topRight(),
                             textRect.bottomLeft(), textRect.bottomRight()}) {
    const auto line = QLineF(corner, mapFromScene(m_port->scenePos()));
    if (line.length() < minLength) {
      shortestLine = line;
      minLength = line.length();
    }
  }

  return shortestLine;
}

void ValueLabel::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
  updateLine();
  Label::mouseMoveEvent(e);
}

void ValueLabel::createLineToPort() {
  if (!m_lineToPort) {
    m_lineToPort = new QGraphicsLineItem(lineToPort(), this);
    m_lineToPort->setZValue(VSRTLScene::Z_ValueLabelHoverLine);
    QPen pen = m_lineToPort->pen();
    pen.setStyle(Qt::DashLine);
    m_lineToPort->setPen(pen);
  }
}

void ValueLabel::hoverEnterEvent(QGraphicsSceneHoverEvent *e) {
  setToolTip(m_port->getTooltipString());
  createLineToPort();
  Label::hoverEnterEvent(e);
}

void ValueLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
  if (!m_showLineToPortAction->isChecked()) {
    delete m_lineToPort;
    m_lineToPort = nullptr;
  }
  Label::hoverLeaveEvent(e);
}

QVariant ValueLabel::itemChange(GraphicsItemChange change,
                                const QVariant &value) {
  if (change == QGraphicsItem::ItemPositionHasChanged) {
    updateLine();
  }
  return Label::itemChange(change, value);
}

void ValueLabel::setLocked(bool) {
  // ValueLabels should always be movable, even when the scene is locked
  return;
}

void ValueLabel::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
  QMenu menu;
  menu.addMenu(createPortRadixMenu(m_port->getPort(), *m_radix));
  menu.addAction(m_visibilityAction.get());
  menu.addAction(m_showLineToPortAction.get());

  menu.exec(event->screenPos());

  // Schedule an update of the label to register any change in the display type
  updateText();
}

void ValueLabel::updateText() {
  setPlainText(encodePortRadixValue(m_port->getPort(), *m_radix));
  applyFormatChanges();
}

} // namespace vsrtl
