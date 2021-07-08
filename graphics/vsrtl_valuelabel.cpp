#include "vsrtl_valuelabel.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_portgraphic.h"

#include <QAction>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

namespace vsrtl {

ValueLabel::ValueLabel(QGraphicsItem* parent, const std::shared_ptr<Radix>& radix, const PortGraphic* port)
    : Label(parent, "", 10), m_radix(radix), m_port(port) {
    setFlag(ItemIsSelectable, true);
    setAcceptHoverEvents(true);
    updateText();
}

void ValueLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* w) {
    // Paint a label box behind the text
    painter->save();
    if (!m_port->getPort()->isConstant()) {
        const bool darkmode = static_cast<VSRTLScene*>(scene())->darkmode();

        QRectF textRect = shape().boundingRect();
        painter->fillRect(textRect, darkmode ? QColor{0x45, 0x45, 0x45} : QColorConstants::White);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(Qt::black, 1));
        painter->drawRect(textRect);
    }
    painter->restore();

    Label::paint(painter, option, w);
}

void ValueLabel::hoverMoveEvent(QGraphicsSceneHoverEvent*) {
    setToolTip(m_port->getTooltipString());
}

void ValueLabel::setLocked(bool) {
    // ValueLabels should always be movable, even when the scene is locked
    return;
}

void ValueLabel::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    menu.addMenu(createPortRadixMenu(m_port->getPort(), *m_radix));

    QAction* showLabel = menu.addAction("Show value");
    showLabel->setCheckable(true);
    showLabel->setChecked(isVisible());
    QObject::connect(showLabel, &QAction::triggered, [this](bool checked) { setVisible(checked); });
    menu.addAction(showLabel);

    menu.exec(event->screenPos());

    // Schedule an update of the label to register any change in the display type
    updateText();
}

void ValueLabel::updateText() {
    setPlainText(encodePortRadixValue(m_port->getPort(), *m_radix));
    applyFormatChanges();
}

}  // namespace vsrtl
