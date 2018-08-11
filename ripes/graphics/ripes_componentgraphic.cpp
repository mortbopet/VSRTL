#include "ripes_componentgraphic.h"

#include "ripes_graphics_defines.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace ripes {

ComponentGraphic::ComponentGraphic(Component* c) : m_component(c) {}

void ComponentGraphic::initialize() {
    Q_ASSERT(scene() != nullptr);

    m_displayText = QString::fromStdString(m_component->getDisplayName());
    m_font = QFont("Times", 10);

    // Setup expand button
    m_expandButton = new QToolButton();
    m_expandButton->setCheckable(true);
    QObject::connect(m_expandButton, &QToolButton::toggled, [=](bool state) { setExpandState(state); });
    m_expandButtonProxy = scene()->addWidget(m_expandButton);

    m_baseRect = calculateBaseRect();
    setExpandState(false);
}

void ComponentGraphic::setPosition(const QPointF& pos) {
    Q_ASSERT(scene() != nullptr);
    m_expandButtonProxy->setPos(pos);
    setPos(pos);
}

void ComponentGraphic::setExpandState(bool expanded) {
    if (!expanded) {
        m_expandButton->setIcon(QIcon(":/icons/plus.svg"));
    } else {
        m_expandButton->setIcon(QIcon(":/icons/minus.svg"));
    }
}

QRectF ComponentGraphic::calculateBaseRect() {
    QRectF baseRect(0, 0, TOP_MARGIN + BOT_MARGIN, SIDE_MARGIN * 2);

    // Calculate text width
    QFontMetrics fm(m_font);
    auto textRect = fm.boundingRect(m_displayText);
    baseRect.adjust(0, 0, textRect.width(), textRect.height());

    // Calculate drawing position of display text
    m_textPos = QPointF(baseRect.width() / 2 - textRect.width() / 2, baseRect.height() / 2 + textRect.height() / 4);

    return baseRect;
}

void ComponentGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    QColor color(Qt::white);
    QColor fillColor = (option->state & QStyle::State_Selected) ? color.dark(150) : color;
    if (option->state & QStyle::State_MouseOver)
        fillColor = fillColor.light(125);

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
    if (lod < 0.2) {
        if (lod < 0.125) {
            painter->fillRect(m_baseRect, fillColor);
            return;
        }

        QBrush b = painter->brush();
        painter->setBrush(fillColor);
        painter->drawRect(m_baseRect);
        painter->setBrush(b);
        return;
    }

    QPen oldPen = painter->pen();
    QPen pen = oldPen;
    int width = 0;
    if (option->state & QStyle::State_Selected)
        width += 2;

    pen.setWidth(width);
    QBrush b = painter->brush();
    painter->setBrush(QBrush(fillColor.dark(option->state & QStyle::State_Sunken ? 120 : 100)));

    painter->drawRect(m_baseRect);
    painter->setBrush(b);

    // Draw shadow
    if (lod >= 1) {
        painter->setPen(QPen(Qt::gray, 1));
        painter->drawLine(m_baseRect.topRight() + QPointF(SHADOW_OFFSET, 0),
                          m_baseRect.bottomRight() + QPointF(SHADOW_OFFSET, SHADOW_OFFSET));
        painter->drawLine(m_baseRect.bottomLeft() + QPointF(0, SHADOW_OFFSET),
                          m_baseRect.bottomRight() + QPointF(SHADOW_OFFSET, SHADOW_OFFSET));
        painter->setPen(QPen(Qt::black, 1));
    }

    // Draw text
    if (lod >= 1.0) {
        painter->setFont(m_font);
        painter->save();
        // painter->scale(0.1, 0.1);
        painter->drawText(m_textPos, m_displayText);
        painter->restore();
    }
}

QRectF ComponentGraphic::boundingRect() const {
    return m_baseRect;
}

void ComponentGraphic::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mousePressEvent(event);
    update();
}

void ComponentGraphic::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mouseMoveEvent(event);
}

void ComponentGraphic::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    QGraphicsItem::mouseReleaseEvent(event);
    update();
}
}
