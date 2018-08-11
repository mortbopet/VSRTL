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

    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);

    m_displayText = QString::fromStdString(m_component->getDisplayName());
    m_font = QFont("Times", 10);

    // Setup expand button
    m_expandButton = new QToolButton();
    m_expandButton->setCheckable(true);
    QObject::connect(m_expandButton, &QToolButton::toggled, [=](bool state) { setExpandState(state); });
    m_expandButtonProxy = scene()->addWidget(m_expandButton);

    setExpandState(false);

    /** @warning calculateBaseRect should always be last! */
    calculateBaseRect();
}

void ComponentGraphic::moveChildrenToAnchor(const QPointF& pos) {
    // Move button
    m_expandButtonProxy->setPos(pos + m_expandButtonPos);
}

QVariant ComponentGraphic::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange) {
        moveChildrenToAnchor(value.toPointF());
    }
    return QGraphicsItem::itemChange(change, value);
}

void ComponentGraphic::setPosition(const QPointF& pos) {
    Q_ASSERT(scene() != nullptr);
    moveChildrenToAnchor(pos);
    setPos(pos);
}

void ComponentGraphic::setExpandState(bool expanded) {
    m_isExpanded = expanded;

    if (!m_isExpanded) {
        m_expandButton->setIcon(QIcon(":/icons/plus.svg"));
    } else {
        m_expandButton->setIcon(QIcon(":/icons/minus.svg"));
    }

    // Recalculate geometry based on now showing child components
    calculateTextPosition();

    update();
}

void ComponentGraphic::calculateTextPosition() {
    QPointF basePos(m_baseRect.width() / 2 - m_textRect.width() / 2, 0);
    if (m_isExpanded) {
        // Move text to top of component to make space for subcomponents
        basePos.setY(BUTTON_INDENT + m_textRect.height());
    } else {
        basePos.setY(m_baseRect.height() / 2);
    }
    m_textPos = basePos;
}

void ComponentGraphic::calculateBaseRect() {
    // ------------------ Base rect ------------------------
    QRectF baseRect(0, 0, TOP_MARGIN + BOT_MARGIN, SIDE_MARGIN * 2);

    // Calculate text width
    QFontMetrics fm(m_font);
    m_textRect = fm.boundingRect(m_displayText);
    baseRect.adjust(0, 0, m_textRect.width(), m_textRect.height());

    // Include expand button in baserect sizing
    baseRect.adjust(0, 0, m_expandButtonProxy->boundingRect().width(), m_expandButtonProxy->boundingRect().height());

    m_baseRect = baseRect;
    // ------------------ Post Base rect ------------------------
    m_expandButtonPos = QPointF(BUTTON_INDENT, BUTTON_INDENT);
    calculateTextPosition();

    // ------------------ Bounding rect ------------------------
    m_boundingRect = baseRect;
    // Adjust for a potential shadow
    m_boundingRect.adjust(0, 0, SHADOW_OFFSET + SHADOW_WIDTH, SHADOW_OFFSET + SHADOW_WIDTH);
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
    if (lod >= 0.5) {
        painter->setPen(QPen(Qt::gray, SHADOW_WIDTH));
        painter->drawLine(m_baseRect.topRight() + QPointF(SHADOW_OFFSET, 0),
                          m_baseRect.bottomRight() + QPointF(SHADOW_OFFSET, SHADOW_OFFSET));
        painter->drawLine(m_baseRect.bottomLeft() + QPointF(0, SHADOW_OFFSET),
                          m_baseRect.bottomRight() + QPointF(SHADOW_OFFSET, SHADOW_OFFSET));
        painter->setPen(QPen(Qt::black, 1));
    }

    // Draw text
    if (lod >= 0.35) {
        painter->setFont(m_font);
        painter->save();
        // painter->scale(0.1, 0.1);
        painter->drawText(m_textPos, m_displayText);
        painter->restore();
    }

    // Determine whether expand button should be shown
    if (lod >= 0.35) {
        m_expandButtonProxy->show();
    } else {
        m_expandButtonProxy->hide();
    }
}

QRectF ComponentGraphic::boundingRect() const {
    return m_boundingRect;
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
