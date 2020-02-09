#include "vsrtl_componentgraphic.h"

#include "vsrtl_componentbutton.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_label.h"
#include "vsrtl_multiplexergraphic.h"
#include "vsrtl_placeroute.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_scene.h"
#include "vsrtl_wiregraphic.h"

#include <cereal/archives/json.hpp>

#include <qmath.h>
#include <deque>
#include <fstream>

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMatrix>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

static constexpr qreal c_resizeMargin = GRID_SIZE;

ComponentGraphic::ComponentGraphic(SimComponent* c, ComponentGraphic* parent) : GridComponent(c, parent) {
    c->changed.Connect(this, &ComponentGraphic::updateSlot);
    c->registerGraphic(this);
    verifySpecialSignals();
}

void ComponentGraphic::verifySpecialSignals() const {
    // Ensure that all special signals required by the graphics type of this component has been set by the simulator
    // component
    auto* type = m_component->getGraphicsType();
    for (const auto& typeID : type->specialPortIDs()) {
        if (m_component->getSpecialPort(typeID) == nullptr) {
            m_component->throwError(
                "Special port: '" + std::string(typeID) +
                "' not assigned. A special port of this ID should be registered through SimComponent::setSpecialPort");
        }
    }
}

void ComponentGraphic::initialize() {
    Q_ASSERT(scene() != nullptr);

    setFlags(ItemIsSelectable | ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);
    setMoveable();

    m_label = new Label(QString::fromStdString(m_component->getDisplayName()), this);

    // Create IO ports of Component
    for (const auto& p_in : m_component->getPorts<SimPort::Direction::in, SimPort>()) {
        m_inputPorts[p_in] = new PortGraphic(p_in, PortType::in, this);
    }
    for (const auto& p_out : m_component->getPorts<SimPort::Direction::out, SimPort>()) {
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

    connect(this, &GridComponent::gridRectChanged, this, &ComponentGraphic::updateGeometry);
    connect(this, &GridComponent::portPosChanged, this, &ComponentGraphic::handlePortPosChanged);
    connect(this, &GridComponent::gridPosChanged, this, &ComponentGraphic::handleGridPosChange);

    // By default, a component is collapsed. This has no effect if a component does not have any subcomponents
    setExpanded(false);
    m_restrictSubcomponentPositioning = true;

    updateGeometry();
    spreadPorts();
}

/**
 * @brief ComponentGraphic::createSubcomponents
 * In charge of hide()ing subcomponents if the parent component (this) is not expanded
 */
void ComponentGraphic::createSubcomponents() {
    for (const auto& c : m_component->getSubComponents()) {
        ComponentGraphic* nc;
        auto typeId = c->getGraphicsID();
        if (typeId == GraphicsIDFor(Multiplexer)) {
            nc = new MultiplexerGraphic(c, this);
        } else if (typeId == GraphicsIDFor(Constant)) {
            // Don't create a distinct ComponentGraphic for constants - these will be drawn next to the port connecting
            // to it
            continue;
        } else {
            nc = new ComponentGraphic(c, this);
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

void ComponentGraphic::loadLayoutFile(const QString& fileName) {
    std::ifstream file(fileName.toStdString());
    cereal::JSONInputArchive archive(file);
    m_isTopLevelSerializedComponent = true;

    try {
        archive(cereal::make_nvp("ComponentGraphic", *this));
    } catch (cereal::Exception e) {
        /// @todo: build an error report
    }

    m_isTopLevelSerializedComponent = false;
    file.close();
}

void ComponentGraphic::loadLayout() {
    QString fileName = QFileDialog::getOpenFileName(QApplication::activeWindow(),
                                                    "Save Layout " + QString::fromStdString(m_component->getName()),
                                                    QString(), tr("JSON (*.json)"));

    if (fileName.isEmpty())
        return;

    loadLayoutFile(fileName);
}

void ComponentGraphic::saveLayout() {
    QString fileName = QFileDialog::getSaveFileName(QApplication::activeWindow(),
                                                    "Save Layout " + QString::fromStdString(m_component->getName()),
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
    try {
        archive(cereal::make_nvp("ComponentGraphic", *this));
    } catch (cereal::Exception e) {
        /// @todo: build an error report
    }
    m_isTopLevelSerializedComponent = false;
}

void ComponentGraphic::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;

    if (hasSubcomponents() && !isLocked()) {
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
                c->setValueLabelVisible(true);
        });
        connect(hideOutputsAction, &QAction::triggered, [=] {
            for (auto& c : m_outputPorts)
                c->setValueLabelVisible(false);
        });

        if (!isLocked()) {
            auto* hiddenPortsMenu = portMenu->addMenu("Hidden ports");
            for (const auto& p : m_component->getAllPorts()) {
                auto* gp = p->getGraphic<PortGraphic>();
                if (gp->userHidden()) {
                    auto* showPortAction = hiddenPortsMenu->addAction(QString::fromStdString(p->getName()));
                    connect(showPortAction, &QAction::triggered, [=] { gp->setUserVisible(true); });
                }
            }
            if (hiddenPortsMenu->actions().size() == 0) {
                delete hiddenPortsMenu;
            }
        }
    }

    // ======================== Indicators menu ============================ //
    if ((m_inputPorts.size() > 0) && !isLocked()) {
        auto* indicatorMenu = menu.addMenu("Indicators");
        for (const auto& portmap : {m_inputPorts, m_outputPorts}) {
            for (const auto& p : portmap) {
                // Value indicators for boolean signals. Boolean indicators will be visible even if the port responsible
                // for the indicator gets hidden.
                if (p->getPort()->getWidth() == 1) {
                    auto* indicatorAction = indicatorMenu->addAction(QString::fromStdString(p->getPort()->getName()));
                    indicatorAction->setCheckable(true);
                    indicatorAction->setChecked(m_indicators.count(p));
                    connect(indicatorAction, &QAction::triggered, [=](bool checked) { setIndicatorState(p, checked); });
                }
            }
        }
        if (indicatorMenu->actions().size() == 0) {
            delete indicatorMenu;
        }
    }

    if ((m_component->getParent() != nullptr) && !isLocked()) {
        auto* hideAction = menu.addAction("Hide component");
        connect(hideAction, &QAction::triggered, [=] {
            m_userHidden = true;
            this->hide();
        });
    }

    if (!isLocked()) {
        bool labelVisible = m_label->isVisible();
        auto* labelVisibilityAction = menu.addAction(labelVisible ? "Hide label" : "Show label");
        connect(labelVisibilityAction, &QAction::triggered, [=] { m_label->setVisible(!labelVisible); });
    }

    menu.exec(event->screenPos());
}

void ComponentGraphic::setIndicatorState(PortGraphic* p, bool enabled) {
    if (enabled) {
        m_indicators.emplace(p);
    } else {
        m_indicators.erase(p);
    }
    update();
}

void ComponentGraphic::registerWire(WireGraphic* wire) {
    m_wires.push_back(wire);
}

void ComponentGraphic::setExpanded(bool state) {
    GridComponent::setExpanded(state);
    bool areWeExpanded = isExpanded();
    if (m_expandButton != nullptr) {
        m_expandButton->setChecked(areWeExpanded);
        for (const auto& c : m_subcomponents) {
            c->setVisible(areWeExpanded && !c->userHidden());
        }
        // We are not hiding the input ports of a component, because these should always be drawn. However, a input port
        // of an expandable component has wires drawin inside the component, which must be hidden aswell, such that they
        // do not accept mouse events nor are drawn.
        for (const auto& w : m_wires) {
            w->setVisible(areWeExpanded);
        }
    }
}

void ComponentGraphic::setUserVisible(bool visible) {
    m_userHidden = !visible;
    setVisible(visible);
}

ComponentGraphic* ComponentGraphic::getParent() const {
    return dynamic_cast<ComponentGraphic*>(parentItem());
}

void ComponentGraphic::updateGeometry() {
    prepareGeometryChange();
    const QRectF sceneRect = sceneGridRect();
    const QRect& currentGridRect = getCurrentComponentRect();

    // Update the draw shape, scaling it to the current scene size of the component grid rect
    QTransform t;
    t.scale(sceneRect.width(), sceneRect.height());
    m_shape = ShapeRegister::getComponentShape(m_component->getGraphicsID(), t);

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

    // Adjust label position through scaling by the relative size change of the component.
    const auto& lastComponentRect = getLastComponentRect();
    if (lastComponentRect != QRect()) {
        const auto widthScaledChanged = static_cast<qreal>(currentGridRect.width()) / lastComponentRect.width();
        const auto heightScaledChanged = static_cast<qreal>(currentGridRect.height()) / lastComponentRect.height();

        // Scale the positioning of the label, adjusting it accordingly to the component size change
        auto labelPos = m_label->pos();
        labelPos.rx() *= widthScaledChanged;
        labelPos.ry() *= heightScaledChanged;
        m_label->setPos(labelPos);
    } else {
        // First time setting label position. Position label above component.
        m_label->setPos(sceneRect.width() / 2, -m_label->boundingRect().height());
    }
}

bool ComponentGraphic::handlePortGraphicMoveAttempt(const PortGraphic* port, const QPointF& newBorderPos) {
    // Port will report its new position in its portGraphic coordinates. Transfor to this (the port parent) coordinate
    // system and attempt to adjust port position.

    const QPoint adjustedPos = sceneToGrid(mapFromItem(port, newBorderPos).toPoint());
    return adjustPort(port->getPort(), adjustedPos.y());
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

    GraphicsBaseItem::setLocked(locked);
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

    return QGraphicsItem::itemChange(change, value);
}

void ComponentGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* w) {
    painter->save();
    QColor color;
    if (static_cast<VSRTLScene*>(scene())->darkmode()) {
        color = hasSubcomponents() && isExpanded() ? QColor(QColor(Qt::darkGray).darker()) : QColor("#c0cdd1");
    } else {
        color = hasSubcomponents() && isExpanded() ? QColor("#ecf0f1") : QColor(Qt::white);
    }

    QColor fillColor = (option->state & QStyle::State_Selected) ? color.darker(150) : color;
    if (option->state & QStyle::State_MouseOver)
        fillColor = fillColor.lighter(125);

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // Draw component outline
    QPen oldPen = painter->pen();
    QPen pen = oldPen;
    int width = COMPONENT_BORDER_WIDTH;
    if (option->state & QStyle::State_Selected)
        width += 1;

    pen.setWidth(width);
    painter->setBrush(QBrush(fillColor.darker(option->state & QStyle::State_Sunken ? 120 : 100)));
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

    // Paint boolean indicators
    for (const auto& p : m_indicators) {
        if (p->getPort()->uValue()) {
            painter->setBrush(Qt::green);
        } else {
            painter->setBrush(Qt::red);
        }

        constexpr qreal dotSize = 12;
        QRectF chordRect(-dotSize / 2, -dotSize / 2, dotSize, dotSize);
        const bool inPort = p->getPortType() == PortType::in;
        chordRect.translate(mapFromItem(p, inPort ? p->getOutputPoint() : p->getInputPoint()));
        QPen pen = painter->pen();
        pen.setWidth(WIRE_WIDTH - 1);
        painter->setPen(pen);
        painter->drawChord(chordRect, -90 * 16, (inPort ? 1 : -1) * 180 * 16);
    }

    // Paint overlay
    paintOverlay(painter, option, w);

#ifdef VSRTL_DEBUG_DRAW
    painter->save();
    painter->setPen(Qt::green);
    painter->drawRect(sceneGridRect());
    painter->restore();
    DRAW_BOUNDING_RECT(painter)
#endif
    painter->restore();
}

QRectF ComponentGraphic::sceneGridRect() const {
    return gridToScene(getCurrentComponentRect());
}

QPainterPath ComponentGraphic::shape() const {
    QPainterPath s;
    s.addRect(sceneGridRect());
    return s;
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
        const auto oldGridRect = getCurrentComponentRect();
        auto newGridRect = oldGridRect;
        newGridRect.setBottomRight(gridPos);

        adjust(newGridRect);
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
