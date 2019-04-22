#include "vsrtl_componentgraphic.h"

#include "vsrtl_componentbutton.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_label.h"
#include "vsrtl_multiplexergraphic.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_registergraphic.h"
#include "vsrtl_traversal_util.h"

#include <qmath.h>
#include <deque>

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <QDebug>
#include <QMatrix>
#include <QPushButton>

namespace vsrtl {

static constexpr qreal c_resizeMargin = GRID_SIZE;
static constexpr qreal c_collapsedSideMargin = 15;

QMap<std::type_index, ComponentGraphic::Shape> ComponentGraphic::s_componentShapes;

ComponentGraphic::ComponentGraphic(Component& c)
    : m_component(c), m_minGridRect(ComponentGraphic::getComponentMinGridRect(c.getTypeId())) {
    c.changed.Connect(this, &ComponentGraphic::updateSlot);
    c.registerGraphic(this);
}

bool ComponentGraphic::hasSubcomponents() const {
    return m_component.getSubComponents().size() != 0;
}

void ComponentGraphic::initialize() {
    Q_ASSERT(scene() != nullptr);

    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);

    m_label = new Label(QString::fromStdString(m_component.getName()), this);

    // Create IO ports of Component
    for (const auto& c : m_component.getInputs()) {
        m_inputPorts[c.get()] = new PortGraphic(c.get(), PortType::in, this);
    }
    for (const auto& c : m_component.getOutputs()) {
        m_outputPorts[c.get()] = new PortGraphic(c.get(), PortType::out, this);
    }

    if (hasSubcomponents()) {
        // Setup expand button
        m_expandButton = new ComponentButton(this);
        connect(m_expandButton, &ComponentButton::toggled, [this](bool expanded) { setExpanded(expanded); });

        createSubcomponents();
    }

    // By default, a component is collapsed. This has no effect if a component does not have any subcomponents
    setExpanded(false);
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
        m_subcomponents.push_back(nc);
        if (!m_isExpanded) {
            nc->hide();
        }
    }
}

void ComponentGraphic::setGridPos(const QPoint& p) {
    m_gridPos = p;
}

void ComponentGraphic::setExpanded(bool state) {
    m_isExpanded = state;
    GeometryChange changeReason = GeometryChange::None;

    if (hasSubcomponents()) {
        Q_ASSERT(m_expandButton != nullptr);
        m_expandButton->setChecked(m_isExpanded);

        if (!m_isExpanded) {
            changeReason = GeometryChange::Collapse;
            for (const auto& c : m_subcomponents) {
                c->hide();
            }
        } else {
            changeReason = GeometryChange::Expand;
            for (const auto& c : m_subcomponents) {
                c->show();
            }
            placeAndRouteSubcomponents();
        }
    }

    // Recalculate geometry based on now showing child components
    updateGeometry(QRect(), changeReason);
}

void ComponentGraphic::placeAndRouteSubcomponents() {
    pr::PlaceRoute::get().placeAndRoute(m_subcomponents, m_routingRegions, m_netlist);

    // Propagate routings to wires
    for (const auto& net : m_netlist) {
        net.nodes[0].port->setNet(net);
    }
}

ComponentGraphic* ComponentGraphic::getParent() const {
    Q_ASSERT(parentItem() != nullptr);
    return static_cast<ComponentGraphic*>(parentItem());
}

QRect ComponentGraphic::subcomponentBoundingGridRect() const {
    // Calculate bounding rectangle of subcomponents in scene coordinates, convert to grid coordinates, and
    // apply as grid rectangle
    auto sceneBoundingRect = QRectF();
    for (const auto& c : m_subcomponents) {
        sceneBoundingRect =
            boundingRectOfRects<QRectF>(sceneBoundingRect, mapFromItem(c, c->boundingRect()).boundingRect());
    }

    return sceneToGrid(sceneBoundingRect);
}

QRect ComponentGraphic::adjustedMinGridRect(bool includePorts, bool moveToParentGridPos) const {
    // Returns the minimum grid rect of the current component with ports taken into account

    // Add height to component based on the largest number of input or output ports. There should always be a
    // margin of 1 on top- and bottom of component
    QRect adjustedRect = m_minGridRect;
    const int largestPortSize = m_inputPorts.size() > m_outputPorts.size() ? m_inputPorts.size() : m_outputPorts.size();
    const int heightToAdd = (largestPortSize + 2) - adjustedRect.height();
    if (heightToAdd > 0) {
        adjustedRect.adjust(0, 0, 0, heightToAdd);
    }

    if (includePorts) {
        // To the view of the place/route algorithms, ports reside on the edge of a component - however, this is not how
        // components are drawn.
        if (m_inputPorts.size() > 0)
            adjustedRect.adjust(0, 0, PortGraphic::portGridWidth(), 0);
        if (m_outputPorts.size() > 0)
            adjustedRect.adjust(0, 0, PortGraphic::portGridWidth(), 0);
    }

    // Move to grid position in parent coordinate system
    if (moveToParentGridPos) {
        adjustedRect.moveTo(m_gridPos);
    }

    return adjustedRect;
}

void ComponentGraphic::updateGeometry(QRect newGridRect, GeometryChange flag) {
    // Rect will change when expanding, so notify canvas that the rect of the current component will be dirty
    prepareGeometryChange();

    // Assert that components without subcomponents cannot be requested to expand or collapse
    Q_ASSERT(!((flag == GeometryChange::Expand || flag == GeometryChange::Collapse) && !hasSubcomponents()));

    // ================= Grid rect sizing ========================= //
    // move operation has already resized base rect to a valid size

    switch (flag) {
        case GeometryChange::Collapse:
        case GeometryChange::None: {
            m_gridRect = adjustedMinGridRect(false, false);

            // Add width to the component based on the name of the component - we define that 2 characters is equal to 1
            // grid spacing : todo; this ratio should be configurable
            m_gridRect.adjust(0, 0, m_component.getName().length() / GRID_SIZE, 0);
            break;
        }
        case GeometryChange::Resize: {
            if (snapToMinGridRect(newGridRect)) {
                m_gridRect = newGridRect;
            } else {
                return;
            }
            break;
        }
        case GeometryChange::Expand:
        case GeometryChange::ChildJustCollapsed:
        case GeometryChange::ChildJustExpanded: {
            m_gridRect = subcomponentBoundingGridRect();
            break;
        }
    }

    // =========================== Scene item positioning ====================== //
    const QRectF sceneRect = sceneGridRect();

    // 1. Set Input/Output port positions
    if (/*m_isExpanded*/ false) {
        // Some fancy logic for positioning IO positions in the best way to facilitate nice signal lines between
        // components
    } else {
        // Component is unexpanded - IO should be positionen in even positions.
        // +2 to ensure a 1 grid margin at the top and bottom of the component
        int i = 0;
        const qreal in_seg_y = sceneRect.height() / (m_inputPorts.size());
        for (const auto& p : m_inputPorts) {
            int gridIndex = roundUp((i * in_seg_y + in_seg_y / 2), GRID_SIZE) / GRID_SIZE;
            p->setGridIndex(gridIndex);
            const qreal y = gridIndex * GRID_SIZE - GRID_SIZE / 2;
            p->setPos(QPointF(sceneRect.left() - GRID_SIZE * PortGraphic::portGridWidth(), y));
            i++;
        }
        i = 0;
        const qreal out_seg_y = sceneRect.height() / (m_outputPorts.size());
        for (const auto& p : m_outputPorts) {
            const int gridIndex = roundUp((i * out_seg_y + out_seg_y / 2), GRID_SIZE) / GRID_SIZE;
            p->setGridIndex(gridIndex);
            const qreal y = gridIndex * GRID_SIZE - GRID_SIZE / 2;
            p->setPos(QPointF(sceneRect.right(), y));
            i++;
        }
    }

    // 2. Set label position
    m_label->setPos(sceneRect.width() / 2, 0);

    // 3 .Update the draw shape, scaling it to the current scene size of the component
    QTransform t;
    t.scale(sceneRect.width(), sceneRect.height());
    t.translate(sceneRect.topLeft().x(), sceneRect.topLeft().y());
    m_shape = ComponentGraphic::getComponentShape(m_component.getTypeId(), t);

    // 5. Position the expand-button
    if (hasSubcomponents()) {
        if (m_isExpanded) {
            m_expandButton->setPos(QPointF(0, 0));
        } else {
            // Center
            const qreal x = sceneRect.width() / 2 - m_expandButton->boundingRect().width() / 2;
            const qreal y = sceneRect.height() / 2 - m_expandButton->boundingRect().height() / 2;
            m_expandButton->setPos(QPointF(x, y));
        }
    }

    // 4. If we have a parent, it should now update its geometry based on new size of its subcomponent(s)
    if (parentItem() && (flag == GeometryChange::Expand || flag == GeometryChange::Collapse)) {
        getParent()->updateGeometry(QRect(), flag == GeometryChange::Expand ? GeometryChange::ChildJustExpanded
                                                                            : GeometryChange::ChildJustCollapsed);
    }
}

QVariant ComponentGraphic::itemChange(GraphicsItemChange change, const QVariant& value) {
    // @todo implement snapping inside parent component
    if (change == ItemPositionChange && scene()) {
        // Output port wires are implicitely redrawn given that the wire is a child of $this. We need to manually signal
        // the wires going to the input ports of this component, to redraw
        if (m_initialized) {
            for (const auto& inputPort : m_inputPorts) {
                getInputPortGraphic<PortGraphic*>(inputPort->getPort())->updateWireGeometry();
            }
        }

        // Snap to grid
        QPointF newPos = value.toPointF();
        qreal x = round(newPos.x() / GRID_SIZE) * GRID_SIZE;
        qreal y = round(newPos.y() / GRID_SIZE) * GRID_SIZE;
        return QPointF(x, y);
        /*
        // Restrict position changes to inside parent item
        const auto& parentRect = getParent()->sceneGridRect;
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

    if (hasSubcomponents()) {
        // Determine whether expand button should be shown
        if (lod >= 0.35) {
            m_expandButton->show();
        } else {
            m_expandButton->hide();
        }
    }

    paintOverlay(painter, option, w);

#ifdef VSRTL_DEBUG_DRAW
    // Bounding rect
    painter->save();
    painter->setPen(Qt::green);
    painter->drawRect(sceneGridRect());
    painter->restore();
    DRAW_BOUNDING_RECT(painter)

    // Routing regions
    if (hasSubcomponents() && m_isExpanded) {
        painter->save();
        painter->setPen(Qt::red);
        for (const auto& rr : m_routingRegions) {
            auto sceneGridRect = gridToScene(rr.get()->r);
            sceneGridRect.moveTo(rr.get()->r.topLeft() * GRID_SIZE);
            painter->drawRect(sceneGridRect);
        }
        painter->restore();
    }
#endif
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
 * @brief Adjust bounds of r to snap on the boundaries of the minimum grid rectangle, or if the component is
 * currently expanded, a rect which contains the subcomponents
 */
bool ComponentGraphic::snapToMinGridRect(QRect& r) const {
    bool snap_r, snap_b;
    snap_r = false;
    snap_b = false;

    const auto& cmpRect =
        hasSubcomponents() && isExpanded() ? subcomponentBoundingGridRect() : adjustedMinGridRect(false, false);

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

QRectF ComponentGraphic::sceneGridRect() const {
    return gridToScene(m_gridRect);
}

QRectF ComponentGraphic::boundingRect() const {
    QRectF boundingRect = sceneGridRect();

    // Adjust slightly for stuff such as shadows, pen sizes etc.
    boundingRect.adjust(-SIDE_MARGIN, -SIDE_MARGIN, SIDE_MARGIN, SIDE_MARGIN);

    return boundingRect;
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
        QPoint gridPos = (event->pos() / GRID_SIZE).toPoint();
        auto newGridRect = m_gridRect;
        newGridRect.setBottomRight(gridPos);
        updateGeometry(newGridRect, GeometryChange::Resize);
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void ComponentGraphic::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    setFlags(flags() | ItemIsMovable);
    m_resizeDragging = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

void ComponentGraphic::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    const auto& sceneRect = sceneGridRect();
    if (sceneRect.width() - event->pos().x() <= c_resizeMargin &&
        sceneRect.height() - event->pos().y() <= c_resizeMargin) {
        this->setCursor(Qt::SizeFDiagCursor);
        m_inResizeDragZone = true;
    } else {
        this->setCursor(Qt::ArrowCursor);
        m_inResizeDragZone = false;
    }
}
}  // namespace vsrtl
