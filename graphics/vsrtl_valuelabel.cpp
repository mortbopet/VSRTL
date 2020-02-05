#include "vsrtl_valuelabel.h"

#include "vsrtl_graphics_util.h"

#include <QAction>
#include <QFontMetrics>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

namespace vsrtl {

QFont ValueLabel::s_font = QFont("Monospace", 8);
QFont ValueLabel::s_constantFont = QFont("Monospace", 10);

static QRectF getTextRect(const QFont& font, const QString& text) {
    QFontMetrics metric = QFontMetrics(font);
    return metric.boundingRect(text);
}

ValueLabel::ValueLabel(Radix& type, const SimPort* port, QGraphicsItem* parent)
    : GraphicsBase(parent), m_type(type), m_port(port) {
    setFlags(ItemIsSelectable);
    setMoveable();
}

QRectF ValueLabel::boundingRect() const {
    auto textRect = getTextRect(s_font, m_text);
    textRect.adjust(-3, -3, 3, 3);

    return textRect;
}

void ValueLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    if (!m_port->isConstant()) {
        QRectF textRect = getTextRect(s_font, m_text);
        textRect.adjust(-1, -1, 1, 1);  // Add a bit of margin to the label
        painter->fillRect(textRect, Qt::white);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(Qt::black, 1));
        painter->drawRect(textRect);
        painter->setFont(s_font);
    } else {
        painter->setFont(s_constantFont);
    }
    painter->drawText(QPointF{0, 0}, m_text);

    painter->restore();
}

void ValueLabel::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    menu.addMenu(createPortRadixMenu(m_port, m_type));

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
    prepareGeometryChange();
    m_text = encodePortRadixValue(m_port, m_type);
}

}  // namespace vsrtl
