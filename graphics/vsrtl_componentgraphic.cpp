#include "vsrtl_componentgraphic.h"

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"

#include <qmath.h>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

ComponentGraphic::ComponentGraphic(Component* c) : m_component(c) {}

void ComponentGraphic::initialize() {
    Q_ASSERT(scene() != nullptr);

    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);

    m_displayText = QString::fromStdString(m_component->getDisplayName());
    m_font = QFont("Times", 10);

    // Get IO of Component
    for (const auto& c : m_component->getInputs()) {
        m_inputPositionMap[c] = QPointF();
    }
    for (const auto& c : m_component->getOutputs()) {
        m_outputPositionMap[c] = QPointF();
    }

    m_hasSubcomponents = m_component->getSubComponents().size() > 0;
    if (m_hasSubcomponents) {
        // Setup expand button
        m_expandButton = new QToolButton();
        m_expandButton->setCheckable(true);
        QObject::connect(m_expandButton, &QToolButton::toggled, [=](bool state) { setExpandState(state); });
        m_expandButtonProxy = scene()->addWidget(m_expandButton);
        m_expandButtonProxy->setParentItem(this);
        m_expandButtonProxy->setPos(QPointF(BUTTON_INDENT, BUTTON_INDENT));

        setExpandState(false);
    }

    createSubcomponents();

    calculateGeometry(Collapse);
}

/**
 * @brief ComponentGraphic::createSubcomponents
 * In charge of hide()ing subcomponents if the parent component (this) is not expanded
 */
void ComponentGraphic::createSubcomponents() {
    for (auto& c : m_component->getSubComponents()) {
        auto nc = new ComponentGraphic(c);
        scene()->addItem(nc);
        nc->initialize();
        nc->setParentItem(this);
        m_subcomponents.push_back(nc);
        if (!m_isExpanded) {
            nc->hide();
        }
    }
}

void ComponentGraphic::setExpandState(bool expanded) {
    m_isExpanded = expanded;

    GeometryChangeFlag changeReason;

    if (!m_isExpanded) {
        changeReason = Collapse;
        m_savedBaseRect = m_baseRect;
        m_expandButton->setIcon(QIcon(":/icons/plus.svg"));
        for (const auto& c : m_subcomponents) {
            c->hide();
        }
    } else {
        changeReason = Expand;
        m_expandButton->setIcon(QIcon(":/icons/minus.svg"));
        for (const auto& c : m_subcomponents) {
            c->show();
        }
    }

    // Recalculate geometry based on now showing child components
    calculateGeometry(changeReason);
}

void ComponentGraphic::calculateGeometry(GeometryChangeFlag flag) {
    // Rect will change when expanding, so notify canvas that the rect of the current component will be dirty
    prepareGeometryChange();

    // Order matters!
    calculateSubcomponentRect();
    calculateBaseRect(flag);
    calculateBoundingRect();
    calculateTextPosition();
    calculateIOPositions();

    // If we have a parent, they should now recalculate its geometry based on new size of this
    if (parentItem() && flag & Expand) {
        static_cast<ComponentGraphic*>(parentItem())->calculateGeometry(ChildJustExpanded);
    }

    update();
}

void ComponentGraphic::calculateSubcomponentRect() {
    if (m_isExpanded) {
        m_subcomponentRect = QRectF();
        for (const auto& c : m_subcomponents) {
            // Bounding rects of subcomponents can have negative coordinates, but we want m_subcomponentRect to start in
            // (0,0). Normalize to ensure.
            m_subcomponentRect =
                boundingRectOfRects<QRectF>(m_subcomponentRect, mapFromItem(c, c->boundingRect()).boundingRect());
        }
        m_subcomponentRect = normalizeRect(m_subcomponentRect);
    } else {
        m_subcomponentRect = QRectF();
    }
}

QRectF ComponentGraphic::sceneBaseRect() const {
    return baseRect().translated(scenePos());
}

QVariant ComponentGraphic::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && scene() && parentItem() && false) {
        // Restrict position changes to inside parent item
        const QRectF parentRect = static_cast<ComponentGraphic*>(parentItem())->baseRect();
        const QRectF thisRect = boundingRect();
        const QPointF offset = thisRect.topLeft();
        QPointF newPos = value.toPointF();
        if (!parentRect.contains(thisRect.translated(newPos))) {
            // Keep the item inside the scene rect.
            newPos.setX(qMin(parentRect.right() - thisRect.width(), qMax(newPos.x(), parentRect.left())));
            newPos.setY(qMin(parentRect.bottom() - thisRect.height(), qMax(newPos.y(), parentRect.top())));
            return newPos - offset;
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void ComponentGraphic::calculateTextPosition() {
    QPointF basePos(m_baseRect.width() / 2 - m_textRect.width() / 2, 0);
    if (m_isExpanded) {
        // Move text to top of component to make space for subcomponents
        basePos.setY(BUTTON_INDENT + m_textRect.height());
    } else {
        basePos.setY(m_baseRect.height() / 2 + m_textRect.height() / 4);
    }
    m_textPos = basePos;
}

void ComponentGraphic::calculateIOPositions() {
    if (/*m_isExpanded*/ false) {
        // Some fancy logic for positioning IO positions in the best way to facilitate nice signal lines between
        // components
    } else {
        // Component is unexpanded - IO should be positionen in even positions
        int i = 0;
        for (auto& c : m_inputPositionMap) {
            c = QPointF(m_baseRect.left(), (m_baseRect.height() / (m_inputPositionMap.size() + 1)) * (1 + i));
            i++;
        }
        i = 0;
        for (auto& c : m_outputPositionMap) {
            c = QPointF(m_baseRect.right(), (m_baseRect.height() / (m_outputPositionMap.size() + 1)) * (1 + i));
            i++;
        }
    }
}

void ComponentGraphic::calculateBaseRect(GeometryChangeFlag flag) {
    if (flag == Resize) {
        // move operation has already resized base rect to a valid size
        return;
    }
    if (flag == Expand && m_savedBaseRect != QRectF()) {
        // Component has previously been expanded and potentially resized - just use the stored base rect size
        m_baseRect = m_savedBaseRect;
        return;
    }

    if (flag == ChildJustExpanded) {
        if (!rectContainsAllSubcomponents(m_baseRect)) {
            calculateSubcomponentRect();
            m_baseRect.setBottomRight(m_subcomponentRect.bottomRight());
        }
        return;
    }

    m_baseRect = QRectF(0, 0, TOP_MARGIN + BOT_MARGIN, SIDE_MARGIN * 2);

    // Calculate text width
    QFontMetrics fm(m_font);
    m_textRect = fm.boundingRect(m_displayText);
    m_baseRect.adjust(0, 0, m_textRect.width(), m_textRect.height());

    // Include expand button in baserect sizing
    if (m_hasSubcomponents) {
        m_baseRect.adjust(0, 0, m_expandButtonProxy->boundingRect().width(),
                          m_expandButtonProxy->boundingRect().height());
    }

    // Adjust for size of the subcomponent rectangle
    if (m_isExpanded) {
        const auto dx = m_baseRect.width() - m_subcomponentRect.width() - SIDE_MARGIN * 2;
        const auto dy =
            m_baseRect.height() - m_subcomponentRect.height() - TOP_MARGIN - BOT_MARGIN - m_textRect.height();

        // expand baseRect to fix subcomponent rect - this is the smallest possible base rect size, used for
        m_baseRect.adjust(0, 0, dx < 0 ? -dx : 0, dy < 0 ? -dy : 0);
    }

    // ------------------ Post Base rect ------------------------
    m_expandButtonPos = QPointF(BUTTON_INDENT, BUTTON_INDENT);
}

void ComponentGraphic::calculateBoundingRect() {
    m_boundingRect = m_baseRect;
    // Adjust for a potential shadow
    m_boundingRect.adjust(0, 0, SHADOW_OFFSET + SHADOW_WIDTH, SHADOW_OFFSET + SHADOW_WIDTH);

    // Adjust for IO pins
    m_boundingRect.adjust(-IO_PIN_LEN, 0, IO_PIN_LEN, 0);
}

void ComponentGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    QColor color = m_hasSubcomponents ? QColor("#ecf0f1") : QColor(Qt::white);
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

    // Draw IO markers
    if (lod >= 0.5) {
        painter->setPen(QPen(Qt::black, 1));
        for (const auto& p : m_inputPositionMap) {
            painter->drawLine(p, p - QPointF(IO_PIN_LEN, 0));
        }
        for (const auto& p : m_outputPositionMap) {
            painter->drawLine(p, p + QPointF(IO_PIN_LEN, 0));
        }
    }
    /*
        // DEBUG: draw bounding rect and base rect
        painter->setPen(QPen(Qt::red, 1));
        painter->drawRect(boundingRect());
        painter->setPen(oldPen);
    */
    if (m_hasSubcomponents) {
        // Determine whether expand button should be shown
        if (lod >= 0.35) {
            m_expandButtonProxy->show();
        } else {
            m_expandButtonProxy->hide();
        }
    }
}

bool ComponentGraphic::rectContainsAllSubcomponents(const QRectF& r) const {
    bool allInside = true;
    for (const auto& rect : m_subcomponents) {
        auto r2 = mapFromItem(rect, rect->boundingRect()).boundingRect();
        allInside &= r.contains(r2);
    }
    return allInside;
}

/**
 * @brief Adjust bounds of r to snap on the boundaries of the subcomponent rect
 * @param r: new rect to check against m_subcomponentRect
 * @return is r different from the subcomponent rect
 */
bool ComponentGraphic::snapToSubcomponentRect(QRectF& r) const {
    bool snap_r, snap_b;
    snap_r = false;
    snap_b = false;

    if (r.right() < m_subcomponentRect.right()) {
        r.setRight(m_subcomponentRect.right());
        snap_r = true;
    }
    if (r.bottom() < m_subcomponentRect.bottom()) {
        r.setBottom(m_subcomponentRect.bottom());
        snap_b = true;
    }

    return !(snap_r & snap_b);
}

QRectF ComponentGraphic::boundingRect() const {
    return m_boundingRect;
}

void ComponentGraphic::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_inDragZone) {
        // start resize drag
        setFlags(flags() & ~ItemIsMovable);
        m_dragging = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void ComponentGraphic::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_dragging) {
        QPointF pos = event->pos();
        auto newRect = m_baseRect;
        newRect.setBottomRight(pos);
        if (snapToSubcomponentRect(newRect)) {
            m_baseRect = newRect;
            calculateGeometry(Resize);
        }
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void ComponentGraphic::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    setFlags(flags() | ItemIsMovable);
    m_dragging = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

void ComponentGraphic::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (baseRect().width() - event->pos().x() <= RESIZE_CURSOR_MARGIN &&
        baseRect().height() - event->pos().y() <= RESIZE_CURSOR_MARGIN && m_hasSubcomponents && m_isExpanded) {
        this->setCursor(Qt::SizeFDiagCursor);
        m_inDragZone = true;
    } else {
        this->setCursor(Qt::ArrowCursor);
        m_inDragZone = false;
    }
}
}  // namespace vsrtl
