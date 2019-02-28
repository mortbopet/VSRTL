#include "vsrtl_componentgraphic.h"

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_label.h"
#include "vsrtl_multiplexergraphic.h"
#include "vsrtl_placeroute.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_registergraphic.h"

#include <qmath.h>
#include <deque>

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <QDebug>

namespace vsrtl {

static constexpr qreal c_resizeMargin = 6;
static constexpr qreal c_collapsedSideMargin = 15;

QMap<std::type_index, ComponentGraphic::Shape> ComponentGraphic::s_componentShapes;

ComponentGraphic::ComponentGraphic(Component& c)
    : m_component(c), m_minRect(ComponentGraphic::getComponentMinRect(c.getTypeId())) {
    c.changed.Connect(this, &ComponentGraphic::updateSlot);
}

bool ComponentGraphic::hasSubcomponents() const {
    return m_component.getSubComponents().size() != 0;
}

void ComponentGraphic::initialize() {
    Q_ASSERT(scene() != nullptr);

    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);

    m_label = new Label(QString::fromStdString(m_component.getName()), this);

    initializePorts();

    if (hasSubcomponents()) {
        // Setup expand button
        m_expandButton = new QToolButton();
        m_expandButton->setCheckable(true);
        QObject::connect(m_expandButton, &QToolButton::toggled, [this](bool expanded) { setExpanded(expanded); });
        m_expandButtonProxy = scene()->addWidget(m_expandButton);
        m_expandButtonProxy->setParentItem(this);

        createSubcomponents();
        orderSubcomponents();
        setExpanded(false);
    } else {
        updateGeometry(Collapse);
    }
}

void ComponentGraphic::initializePorts() {
    // Create IO ports of Component
    for (const auto& c : m_component.getInputs()) {
        m_inputPorts[c.get()] = new PortGraphic(c.get(), PortType::in, this);
    }
    for (const auto& c : m_component.getOutputs()) {
        m_outputPorts[c.get()] = new PortGraphic(c.get(), PortType::out, this);
    }
}

/**
 * @brief ComponentGraphic::createSubcomponents
 * In charge of hide()ing subcomponents if the parent component (this) is not expanded
 */
void ComponentGraphic::createSubcomponents() {
    for (const auto& c : m_component.getSubComponents()) {
        ComponentGraphic* nc;
        if (dynamic_cast<Multiplexer*>(c.get())) {
            nc = new MultiplexerGraphic(*static_cast<Multiplexer*>(c.get()));
        } else if (dynamic_cast<Register*>(c.get())) {
            nc = new RegisterGraphic(*static_cast<Register*>(c.get()));
        } else {
            nc = new ComponentGraphic(*c);
        }
        scene()->addItem(nc);
        nc->initialize();
        nc->setParentItem(this);
        m_subcomponents[nc] = c.get();
        if (!m_isExpanded) {
            nc->hide();
        }
    }
}

/**
 * @brief orderSubcomponents
 * Subcomponent ordering is based on a topological sort algorithm.
 * This algorithm is applicable for DAG's (Directed Acyclical Graph's). Naturally, digital circuits does not fulfull the
 * requirement for being a DAG. However, if Registers are seen as having either their input or output disregarded as an
 * edge in a DAG, a DAG can be created from a digital circuit, if only outputs are considered.
 * Having a topological sorting of the components, rows- and columns can
 * be generated for each depth in the topological sort, wherein the components are finally layed out.
 * @param parent
 */

namespace {
void orderSubcomponentsUtil(Component* c, std::map<Component*, bool>& visited, std::deque<Component*>& stack) {
    visited[c] = true;

    for (const auto& cc : c->getOutputComponents()) {
        // The find call ensures that the output components are inside this subcomponent. OutputComponents are "parent"
        // agnostic, and has no notion of enclosing components.
        if (visited.find(cc) != visited.end() && !visited[cc]) {
            orderSubcomponentsUtil(cc, visited, stack);
        }
    }
    stack.push_front(c);
}

}  // namespace

template <typename K, typename V>
K reverseLookup(const std::map<K, V>& m, const V& v) {
    for (const auto& i : m) {
        if (i.second == v)
            return i.first;
    }
    return K();
}

void ComponentGraphic::orderSubcomponents() {
    const auto& placements = PlaceRoute::get()->placeAndRoute(m_subcomponents);
    for (const auto& p : placements) {
        p.first->setPos(p.second);
    }
}

void ComponentGraphic::setExpanded(bool expanded) {
    m_isExpanded = expanded;
    m_expandButton->setChecked(m_isExpanded);

    GeometryChangeFlag changeReason;

    if (!m_isExpanded) {
        changeReason = Collapse;
        m_savedBaseRect = m_baseRect;
        m_expandButton->setIcon(QIcon(":/icons/plus.svg"));
        for (const auto& c : m_subcomponents) {
            c.first->hide();
        }
    } else {
        changeReason = Expand;
        m_expandButton->setIcon(QIcon(":/icons/minus.svg"));
        for (const auto& c : m_subcomponents) {
            c.first->show();
        }
    }

    // Recalculate geometry based on now showing child components
    updateGeometry(changeReason);
}

ComponentGraphic* ComponentGraphic::getParent() const {
    Q_ASSERT(parentItem() != nullptr);
    return static_cast<ComponentGraphic*>(parentItem());
}

void ComponentGraphic::updateDrawShape() {
    QTransform t;
    t.scale(m_baseRect.width(), m_baseRect.height());
    m_shape = ComponentGraphic::getComponentShape(m_component.getTypeId(), t);
}

void ComponentGraphic::setLabelPosition() {
    m_label->setPos(m_baseRect.width() / 2, 5);
}

void ComponentGraphic::updateGeometry(GeometryChangeFlag flag) {
    // Rect will change when expanding, so notify canvas that the rect of the current component will be dirty
    prepareGeometryChange();

    // Order matters!
    updateSubcomponentRect();
    updateBaseRect(flag);
    updateBoundingRect();
    updateTextPosition();
    setIOPortPositions();
    setLabelPosition();

    updateDrawShape();

    if (hasSubcomponents()) {
        if (m_isExpanded) {
            m_expandButtonProxy->setPos(QPointF(0, 0));
        } else {
            // Center
            const qreal x = m_baseRect.width() / 2 - m_expandButton->width() / 2;
            const qreal y = m_baseRect.height() / 2 - m_expandButton->height() / 2;
            m_expandButtonProxy->setPos(QPointF(x, y));
        }
    }

    // If we have a parent, it should now update its geometry based on new size of this
    if (parentItem()) {
        getParent()->updateGeometry(flag & Expand ? ChildJustExpanded : ChildJustCollapsed);
    }

    // Schedule a redraw after updating the component geometry
    update();
}

void ComponentGraphic::updateSubcomponentRect() {
    if (m_isExpanded) {
        m_subcomponentRect = QRectF();
        for (const auto& c : m_subcomponents) {
            // Bounding rects of subcomponents can have negative coordinates, but we want m_subcomponentRect to start in
            // (0,0). Normalize to ensure.
            m_subcomponentRect = boundingRectOfRects<QRectF>(
                m_subcomponentRect, mapFromItem(c.first, c.first->boundingRect()).boundingRect());
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
    // @todo implement snapping inside parent component
    if (change == ItemPositionChange && scene()) {
        // Output port wires are implicitely redrawn given that the wire is a child of $this. We need to manually signal
        // the wires going to the input ports of this component, to redraw
        if (m_initialized) {
            for (const auto& inputPort : m_inputPorts) {
                inputPort->updateInputWire();
            }
        }

        // Snap to grid
        QPointF newPos = value.toPointF();
        qreal x = round(newPos.x() / GRID_SIZE) * GRID_SIZE;
        qreal y = round(newPos.y() / GRID_SIZE) * GRID_SIZE;
        return QPointF(x, y);
        /*
        // Restrict position changes to inside parent item
        const auto& parentRect = getParent()->baseRect();
        const auto& thisRect = boundingRect();
        const auto& offset = thisRect.topLeft();
        QPointF newPos = value.toPointF();
        if (!parentRect.contains(thisRect.translated(newPos))) {
            // Keep the item inside the scene rect.
            newPos.setX(qMin(parentRect.right() - thisRect.width(), qMax(newPos.x(), parentRect.left())));
            newPos.setY(qMin(parentRect.bottom() - thisRect.height(), qMax(newPos.y(), parentRect.top())));
            return newPos - offset;
        }
        */
    }

    return QGraphicsItem::itemChange(change, value);
}

void ComponentGraphic::updateTextPosition() {
    QPointF basePos(m_baseRect.width() / 2 - m_textRect.width() / 2, 0);
    if (m_isExpanded) {
        // Move text to top of component to make space for subcomponents
        basePos.setY(BUTTON_INDENT + m_textRect.height());
    } else {
        basePos.setY(m_baseRect.height() / 2 + m_textRect.height() / 4);
    }
    m_textPos = basePos;
}

void ComponentGraphic::setIOPortPositions() {
    if (/*m_isExpanded*/ false) {
        // Some fancy logic for positioning IO positions in the best way to facilitate nice signal lines between
        // components
    } else {
        // Component is unexpanded - IO should be positionen in even positions
        int i = 0;
        const qreal in_seg_y = m_baseRect.height() / m_inputPorts.size();
        for (const auto& c : m_inputPorts) {
            const qreal y = i * in_seg_y + in_seg_y / 2;
            c->setPos(QPointF(m_baseRect.left() - c->boundingRect().width(), y));
            i++;
        }
        i = 0;
        const qreal out_seg_y = m_baseRect.height() / m_outputPorts.size();
        for (const auto& c : m_outputPorts) {
            const qreal y = i * out_seg_y + out_seg_y / 2;
            c->setPos(QPointF(m_baseRect.right(), y));
            i++;
        }
    }
}

void ComponentGraphic::updateBaseRect(GeometryChangeFlag flag) {
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
            updateSubcomponentRect();
            m_baseRect.setBottomRight(m_subcomponentRect.bottomRight());
        }
        return;
    }

    // Recalculate base rect
    m_baseRect = QRectF();

    // Include expand button in baserect sizing
    if (hasSubcomponents()) {
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
    } else if (hasSubcomponents()) {
        // Expand rect to have padding between expand button and edges
        m_baseRect.adjust(0, 0, c_collapsedSideMargin, c_collapsedSideMargin);
    }

    // Adjust for Ports
    const auto& largestPortVector = m_inputPorts.size() > m_outputPorts.size() ? m_inputPorts : m_outputPorts;
    for (const auto& port : largestPortVector) {
        m_baseRect.adjust(0, 0, 0, port->boundingRect().height());
    }

    // Snap to minimum size of rect
    if (m_baseRect.height() < m_minRect.height()) {
        m_baseRect.setHeight(m_minRect.height());
    }
    if (m_baseRect.width() < m_minRect.width()) {
        m_baseRect.setWidth(m_minRect.width());
    }

    // Round up to grid size
    m_baseRect.setHeight(roundUp(m_baseRect.height(), GRID_SIZE));
    m_baseRect.setWidth(roundUp(m_baseRect.width(), GRID_SIZE));

    // ------------------ Post Base rect ------------------------
    m_expandButtonPos = QPointF(BUTTON_INDENT, BUTTON_INDENT);
}

void ComponentGraphic::setShape(const QPainterPath& shape) {
    m_shape = shape;
}

namespace {
qreal largestPortWidth(const QMap<Port*, PortGraphic*>& ports) {
    qreal width = 0;
    for (const auto& port : ports) {
        width = port->boundingRect().width() > width ? port->boundingRect().width() : width;
    }
    return width;
}
}  // namespace

void ComponentGraphic::updateBoundingRect() {
    m_boundingRect = m_baseRect;
    // Adjust for a potential shadow
    m_boundingRect.adjust(0, 0, SHADOW_OFFSET + SHADOW_WIDTH, SHADOW_OFFSET + SHADOW_WIDTH);

    // Adjust for IO pins. IO pins may vary in size, so find the largest
    m_boundingRect.adjust(-largestPortWidth(m_inputPorts), 0, largestPortWidth(m_outputPorts), 0);
}

void ComponentGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* w) {
    QColor color = hasSubcomponents() ? QColor("#ecf0f1") : QColor(Qt::white);
    QColor fillColor = (option->state & QStyle::State_Selected) ? color.dark(150) : color;
    if (option->state & QStyle::State_MouseOver)
        fillColor = fillColor.light(125);

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // Draw component outline
    QPen oldPen = painter->pen();
    QPen pen = oldPen;
    int width = COMPONENT_BORDER_WIDTH;
    if (option->state & QStyle::State_Selected)
        width += 1;

    pen.setWidth(width);
    painter->setBrush(QBrush(fillColor.dark(option->state & QStyle::State_Sunken ? 120 : 100)));
    painter->setPen(pen);
    painter->drawPath(m_shape);

    painter->setPen(oldPen);

    // Draw text
    if (lod >= 0.35) {
        painter->setFont(m_font);
        painter->save();
        // painter->scale(0.1, 0.1);
        painter->drawText(m_textPos, m_displayText);
        painter->restore();
    }

    /*
    // DEBUG: draw bounding rect and base rect
    painter->setPen(QPen(Qt::red, 1));
    painter->drawRect(boundingRect());
    painter->setPen(oldPen);
    */

    if (hasSubcomponents()) {
        // Determine whether expand button should be shown
        if (lod >= 0.35) {
            m_expandButtonProxy->show();
        } else {
            m_expandButtonProxy->hide();
        }
    }

    paintOverlay(painter, option, w);
}

bool ComponentGraphic::rectContainsAllSubcomponents(const QRectF& r) const {
    bool allInside = true;
    for (const auto& rect : m_subcomponents) {
        auto r2 = mapFromItem(rect.first, rect.first->boundingRect()).boundingRect();
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

    const auto& cmpRect = hasSubcomponents() ? m_subcomponentRect : m_minRect;

    if (r.right() < cmpRect.right()) {
        r.setRight(cmpRect.right());
        snap_r = true;
    }
    if (r.bottom() < cmpRect.bottom()) {
        r.setBottom(cmpRect.bottom());
        snap_b = true;
    }

    return !(snap_r & snap_b);
}

QRectF ComponentGraphic::boundingRect() const {
    return m_boundingRect;
}

void ComponentGraphic::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_inResizeDragZone) {
        // start resize drag
        setFlags(flags() & ~ItemIsMovable);
        m_resizeDragging = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void ComponentGraphic::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_resizeDragging) {
        QPointF pos = event->pos();
        roundNear(pos, GRID_SIZE);  // snap to grid
        auto newRect = m_baseRect;
        newRect.setBottomRight(pos);
        if (snapToSubcomponentRect(newRect)) {
            m_baseRect = newRect;
            updateGeometry(Resize);
        }
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void ComponentGraphic::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    setFlags(flags() | ItemIsMovable);
    m_resizeDragging = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

void ComponentGraphic::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (baseRect().width() - event->pos().x() <= c_resizeMargin &&
        baseRect().height() - event->pos().y() <= c_resizeMargin) {
        this->setCursor(Qt::SizeFDiagCursor);
        m_inResizeDragZone = true;
    } else {
        this->setCursor(Qt::ArrowCursor);
        m_inResizeDragZone = false;
    }
}
}  // namespace vsrtl
