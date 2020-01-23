#include "vsrtl_componentgraphic.h"

#include "vsrtl_componentbutton.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_label.h"
#include "vsrtl_multiplexergraphic.h"
#include "vsrtl_placeroute.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_registergraphic.h"
#include "vsrtl_wiregraphic.h"

#include <cereal/archives/json.hpp>

#include <qmath.h>
#include <deque>
#include <fstream>

#include <QApplication>
#include <QFileDialog>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMatrix>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

namespace {

static inline qreal snapToGrid(qreal v) {
    return round(v / GRID_SIZE) * GRID_SIZE;
}

static inline QRectF gridToScene(QRect gridRect) {
    // Scales a rectangle in grid coordinates to scene coordinates
    QRectF sceneGridRect;
    sceneGridRect.setWidth(gridRect.width() * GRID_SIZE);
    sceneGridRect.setHeight(gridRect.height() * GRID_SIZE);
    return sceneGridRect;
}

static inline QPoint sceneToGrid(QPointF p) {
    return (p / GRID_SIZE).toPoint();
}

static inline QPointF gridToScene(QPoint p) {
    return p * GRID_SIZE;
}

}  // namespace

static constexpr qreal c_resizeMargin = GRID_SIZE;
static constexpr qreal c_collapsedSideMargin = 15;

ComponentGraphic::ComponentGraphic(SimComponent& c, ComponentGraphic* parent) : GridComponent(c, parent) {
    c.changed.Connect(this, &ComponentGraphic::updateSlot);
    c.registerGraphic(this);
    verifySpecialSignals();
}

void ComponentGraphic::verifySpecialSignals() const {
    // Ensure that all special signals required by the graphics type of this component has been set by the simulator
    // component
    auto* type = m_component.getGraphicsType();
    for (const auto& typeID : type->specialPortIDs()) {
        if (m_component.getSpecialPort(typeID) == nullptr) {
            m_component.throwError(
                "Special port: '" + std::string(typeID) +
                "' not assigned. A special port of this ID should be registered through SimComponent::setSpecialPort");
        }
    }
}

bool ComponentGraphic::hasSubcomponents() const {
    return m_component.getSubComponents().size() != 0;
}

void ComponentGraphic::initialize() {
    Q_ASSERT(scene() != nullptr);

    setFlags(ItemIsSelectable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);
    setMoveable();

    m_label = new Label(QString::fromStdString(m_component.getDisplayName()), this);

    // Create IO ports of Component
    for (const auto& p_in : m_component.getPorts<SimPort::Direction::in, SimPort>()) {
        m_inputPorts[p_in] = new PortGraphic(p_in, PortType::in, this);
    }
    for (const auto& p_out : m_component.getPorts<SimPort::Direction::out, SimPort>()) {
        m_outputPorts[p_out] = new PortGraphic(p_out, PortType::out, this);
    }

    m_restrictSubcomponentPositioning = false;
    if (hasSubcomponents()) {
        // Setup expand button
        m_expandButton = new ComponentButton(this);
        connect(m_expandButton, &ComponentButton::toggled, [this](bool expanded) { setExpanded(expanded); });

        createSubcomponents();
        placeAndRouteSubcomponents();
    }

    // By default, a component is collapsed. This has no effect if a component does not have any subcomponents
    setExpanded(false);
    m_restrictSubcomponentPositioning = true;

    connect(this, &GridComponent::gridRectChanged, this, &ComponentGraphic::updateGeometry);
    connect(this, &GridComponent::portPosChanged, this, &ComponentGraphic::handlePortPosChanged);
    connect(this, &GridComponent::gridPosChanged, this, &ComponentGraphic::handleGridPosChange);
    spreadPorts();
    updateGeometry();
}

/**
 * @brief ComponentGraphic::createSubcomponents
 * In charge of hide()ing subcomponents if the parent component (this) is not expanded
 */
void ComponentGraphic::createSubcomponents() {
    for (const auto& c : m_component.getSubComponents()) {
        ComponentGraphic* nc;
        auto typeId = c->getGraphicsID();
        if (typeId == GraphicsIDFor(Multiplexer)) {
            nc = new MultiplexerGraphic(*c, this);
        } else if (typeId == GraphicsIDFor(Register)) {
            nc = new RegisterGraphic(*c, this);
        } else if (typeId == GraphicsIDFor(Constant)) {
            // Don't create a distinct ComponentGraphic for constants - these will be drawn next to the port connecting
            // to it
            continue;
        } else {
            nc = new ComponentGraphic(*c, this);
        }
        nc->initialize();
        nc->setParentItem(this);
        nc->m_parentComponentGraphic = this;
        m_subcomponents.push_back(nc);
        if (!isExpanded()) {
            nc->hide();
        }
    }
}

void ComponentGraphic::resetWires() {
    const QString text =
        "Reset wires?\nThis will remove all interconnecting points for all wires within this subcomponent";

    if (QMessageBox::Yes == QMessageBox::question(QApplication::activeWindow(), "Reset wires", text)) {
        // Clear subcomponent wires
        for (const auto& c : m_subcomponents) {
            for (const auto& p : c->outputPorts()) {
                p->getOutputWire()->clearWirePoints();
            }
        }
        // Clear wires from this components input ports
        for (const auto& p : m_inputPorts) {
            p->getOutputWire()->clearWirePoints();
        }
    }
}

void ComponentGraphic::loadLayout() {
    QString fileName = QFileDialog::getOpenFileName(QApplication::activeWindow(),
                                                    "Save Layout " + QString::fromStdString(m_component.getName()),
                                                    QString(), tr("JSON (*.json)"));

    if (fileName.isEmpty())
        return;

    std::ifstream file(fileName.toStdString());
    cereal::JSONInputArchive archive(file);
    m_isTopLevelSerializedComponent = true;
    archive(cereal::make_nvp("ComponentGraphic", *this));
    m_isTopLevelSerializedComponent = false;
}

void ComponentGraphic::saveLayout() {
    QString fileName = QFileDialog::getSaveFileName(QApplication::activeWindow(),
                                                    "Save Layout " + QString::fromStdString(m_component.getName()),
                                                    QString(), tr("JSON (*.json)"));

    if (fileName.isEmpty())
        return;
    if (!fileName.endsWith(".json"))
        fileName += ".json";
    std::ofstream file(fileName.toStdString());
    cereal::JSONOutputArchive archive(file);

    /// @todo: Is it more applicable to do a typeid(getComponent()).name() ? this would not work accross separate
    /// compilers, but would directly indicate the underlying type of which this layout is compatible with...
    m_isTopLevelSerializedComponent = true;
    archive(cereal::make_nvp("ComponentGraphic", *this));
    m_isTopLevelSerializedComponent = false;
}

void ComponentGraphic::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (isLocked())
        return;

    QMenu menu;

    if (hasSubcomponents()) {
        // ======================== Layout menu ============================ //
        auto* layoutMenu = menu.addMenu("Layout");
        auto* loadAction = layoutMenu->addAction("Load layout");
        auto* saveAction = layoutMenu->addAction("Save layout");
        auto* resetWiresAction = layoutMenu->addAction("Reset wires");

        connect(saveAction, &QAction::triggered, this, &ComponentGraphic::saveLayout);
        connect(loadAction, &QAction::triggered, this, &ComponentGraphic::loadLayout);
        connect(resetWiresAction, &QAction::triggered, this, &ComponentGraphic::resetWires);
    }

    if (m_outputPorts.size() > 0) {
        // ======================== Ports menu ============================ //
        auto* portMenu = menu.addMenu("Ports");
        auto* showOutputsAction = portMenu->addAction("Show output values");
        auto* hideOutputsAction = portMenu->addAction("Hide output values");
        connect(showOutputsAction, &QAction::triggered, [=] {
            for (auto& c : m_outputPorts)
                c->setLabelVisible(true);
        });
        connect(hideOutputsAction, &QAction::triggered, [=] {
            for (auto& c : m_outputPorts)
                c->setLabelVisible(false);
        });
    }

    if (m_component.getParent() != nullptr) {
        auto* hideAction = menu.addAction("Hide");
        connect(hideAction, &QAction::triggered, [=] { this->hide(); });
    }

    menu.exec(event->screenPos());
}

void ComponentGraphic::setExpanded(bool state) {
    GridComponent::setExpanded(state);
    bool areWeExpanded = isExpanded();
    if (m_expandButton != nullptr) {
        m_expandButton->setChecked(areWeExpanded);
        for (const auto& c : m_subcomponents) {
            c->setVisible(areWeExpanded);
        }
        // We are not hiding the input ports of a component, because these should always be drawn. However, a input port
        // of an expandable component has wires drawin inside the component, which must be hidden aswell, such that they
        // do not accept mouse events nor are drawn.
        for (const auto& p : m_inputPorts) {
            p->setOutwireVisible(areWeExpanded);
        }
    }
}

ComponentGraphic* ComponentGraphic::getParent() const {
    return dynamic_cast<ComponentGraphic*>(parentItem());
}

void ComponentGraphic::updateGeometry() {
    prepareGeometryChange();
    const QRectF sceneRect = sceneGridRect();

    // Set label position
    m_label->setPos(sceneRect.width() / 2, 0);

    // Update the draw shape, scaling it to the current scene size of the component grid rect
    QTransform t;
    t.scale(sceneRect.width(), sceneRect.height());
    m_shape = ShapeRegister::getComponentShape(m_component.getGraphicsID(), t);

    // Position the expand-button
    if (hasSubcomponents()) {
        if (isExpanded()) {
            m_expandButton->setPos(QPointF(0, 0));
        } else {
            // Center
            const qreal x = sceneRect.width() / 2 - m_expandButton->boundingRect().width() / 2;
            const qreal y = sceneRect.height() / 2 - m_expandButton->boundingRect().height() / 2;
            m_expandButton->setPos(QPointF(x, y));
        }
    }

    // Update the grid points within this component, if it has subcomponents
    if (hasSubcomponents() && isExpanded()) {
        // Grid should only be drawing inside the component, so remove 1 gridsize from each edge of the
        // component rect
        auto rect = m_shape.boundingRect();
        QPoint gridTopLeft = (rect.topLeft() / GRID_SIZE).toPoint() * GRID_SIZE;
        gridTopLeft += QPoint(GRID_SIZE, GRID_SIZE);
        QPoint gridBotRight = (rect.bottomRight() / GRID_SIZE).toPoint() * GRID_SIZE;
        gridBotRight -= QPoint(GRID_SIZE, GRID_SIZE);

        m_gridPoints.clear();
        for (int x = gridTopLeft.x(); x <= gridBotRight.x(); x += GRID_SIZE)
            for (int y = gridTopLeft.y(); y <= gridBotRight.y(); y += GRID_SIZE)
                m_gridPoints << QPoint(x, y);
    }
}

void ComponentGraphic::handleGridPosChange(const QPoint p) {
    setPos(gridToScene(p));
}

void ComponentGraphic::handlePortPosChanged(const SimPort* port) {
    const auto pos = getPortPos(port);
    auto* g = port->getGraphic<PortGraphic>();

    switch (pos.dir) {
        case Side::Left: {
            g->setPos(
                QPointF(sceneGridRect().left() - GRID_SIZE * PortGraphic::portGridWidth(), pos.index * GRID_SIZE));
            break;
        }
        case Side::Right: {
            g->setPos(QPointF(sceneGridRect().right(), pos.index * GRID_SIZE));
            break;
        }
        default:
            Q_UNREACHABLE();
    }
}

void ComponentGraphic::setLocked(bool locked) {
    // No longer give the option of expand the component if the scene is locked
    if (m_expandButton)
        m_expandButton->setVisible(!locked);

    GraphicsBase::setLocked(locked);
}

QVariant ComponentGraphic::itemChange(GraphicsItemChange change, const QVariant& value) {
    // @todo implement snapping inside parent component
    if (change == ItemPositionChange && scene()) {
        // Output port wires are implicitely redrawn given that the wire is a child of $this. We need to manually signal
        // the wires going to the input ports of this component, to redraw
        if (m_initialized) {
            for (const auto& inputPort : m_inputPorts) {
                if (!inputPort->getPort()->isConstant())
                    inputPort->getPort()->getInputPort()->getGraphic<PortGraphic>()->updateWireGeometry();
            }
        }

        if (parentIsPlacing()) {
            // New position has been validated (and set) through the grid layer, and the grid component has >already<
            // been moved to its new position. This slot is called through the move signal of the gridComponent, emitted
            // during place and route.
            return value;
        } else {
            /// Parent is not placing, @p value represents a QPointF in parent scene coordinates. Go through
            /// GridComponent to validate and attempt to place the component.
            const QPoint newGridPos = sceneToGrid(value.toPoint());

            if (move(newGridPos)) {
                return gridToScene(getGridPos());
            } else {
                // Move was unsuccessfull, keep current positioning
                return pos();
            }
        }
    }

    if (change == ItemVisibleChange && scene()) {
        // hide all input ports of other components which this component has output ports connecting to.
        const bool visible = value.toBool();

        for (const auto& p_out : m_component.getPorts<SimPort::Direction::out>()) {
            for (const auto& p_conn : p_out->getOutputPorts()) {
                auto* portParent = p_conn->getParent();
                auto* portGraphic = p_conn->getGraphic<PortGraphic>();

                const bool isNestedComponent = portParent == m_component.getParent();

                if (!isNestedComponent && portParent && portGraphic) {
                    portGraphic->setVisible(visible & portParent->getGraphic<ComponentGraphic>()->isVisible());
                }
            }
        }

        for (const auto& p_in : m_component.getPorts<SimPort::Direction::in>()) {
            p_in->getGraphic<PortGraphic>()->setVisible(visible);
        }
    }

    return QGraphicsItem::itemChange(change, value);
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

    if (hasSubcomponents()) {
        if (lod >= 0.35) {
            // Determine whether expand button should be shown. If we are in locked state, do not interfere with the
            // view state of the expand button
            if (!isLocked()) {
                m_expandButton->show();
            } else {
                m_expandButton->hide();
            }

            if (isExpanded()) {
                // Draw grid
                painter->save();
                painter->setPen(QPen(Qt::lightGray, 1));
                painter->drawPoints(m_gridPoints);
                painter->restore();
            }
        }
    }

    paintOverlay(painter, option, w);

#ifdef VSRTL_DEBUG_DRAW
    painter->save();
    painter->setPen(Qt::green);
    painter->drawRect(sceneGridRect());
    painter->restore();
    DRAW_BOUNDING_RECT(painter)
#endif
}

QRectF ComponentGraphic::sceneGridRect() const {
    return gridToScene(getCurrentComponentRect());
}

QRectF ComponentGraphic::boundingRect() const {
    QRectF boundingRect = sceneGridRect();

    // Adjust slightly for stuff such as shadows, pen sizes etc.
    boundingRect.adjust(-SIDE_MARGIN, -SIDE_MARGIN, SIDE_MARGIN, SIDE_MARGIN);

    return boundingRect;
}

void ComponentGraphic::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (flags().testFlag(ItemIsMovable) && event->button() == Qt::LeftButton && m_inResizeDragZone) {
        // start resize drag
        setFlags(flags() & ~ItemIsMovable);
        m_resizeDragging = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void ComponentGraphic::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_resizeDragging) {
        QPoint gridPos = (event->pos() / GRID_SIZE).toPoint();
        auto newGridRect = getCurrentComponentRect();
        newGridRect.setBottomRight(gridPos);

        if (adjust(newGridRect))
            updateGeometry();
    }

    QGraphicsItem::mouseMoveEvent(event);
}

void ComponentGraphic::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_resizeDragging) {
        setFlags(flags() | ItemIsMovable);
        m_resizeDragging = false;
    }

    QGraphicsItem::mouseReleaseEvent(event);
}

void ComponentGraphic::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (!isLocked()) {
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
}
}  // namespace vsrtl
