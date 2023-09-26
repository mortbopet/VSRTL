#include "vsrtl_componentgraphic.h"

#include "vsrtl_componentbutton.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_label.h"
#include "vsrtl_multiplexergraphic.h"
#include "vsrtl_parameterdialog.h"
#include "vsrtl_placeroute.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_scene.h"
#include "vsrtl_wiregraphic.h"

#include <cereal/archives/json.hpp>

#include <deque>
#include <fstream>
#include <qmath.h>

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QFileDialog>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMatrix4x4>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStyleOptionGraphicsItem>

#include <memory>

namespace vsrtl {

static constexpr qreal c_resizeMargin = GRID_SIZE;

ComponentGraphic::ComponentGraphic(SimComponent *c, ComponentGraphic *parent)
    : GridComponent(c, parent) {
  // Connect changes from simulator through our signal translation mechanism.
  wrapSimSignal(c->changed);
  c->registerGraphic(this);
  verifySpecialSignals();
}

void ComponentGraphic::verifySpecialSignals() const {
  // Ensure that all special signals required by the graphics type of this
  // component has been set by the simulator component
  auto *type = m_component->getGraphicsType();
  if (!type) {
    throw std::runtime_error("No graphics type registerred for component " +
                             m_component->getHierName());
  }
  for (const auto &typeID : type->specialPortIDs()) {
    if (m_component->getSpecialPort(typeID) == nullptr) {
      m_component->throwError(
          "Special port: '" + std::string(typeID) +
          "' not assigned. A special port of this ID should be registered "
          "through SimComponent::setSpecialPort");
    }
  }
}

GraphicsBaseItem<QGraphicsItem> *ComponentGraphic::moduleParent() {
  auto *parent = dynamic_cast<GraphicsBaseItem<QGraphicsItem> *>(parentItem());
  Q_ASSERT(parent);
  return parent;
}

void ComponentGraphic::initialize(bool doPlaceAndRoute) {
  setToolTip(QString::fromStdString(m_component->getDescription()));
  setFlags(ItemIsSelectable | flags());
  setAcceptHoverEvents(true);
  setMoveable();

  m_labelVisibilityAction = std::make_shared<QAction>("Show label");
  m_labelVisibilityAction->setCheckable(true);
  m_labelVisibilityAction->setChecked(true);
  connect(m_labelVisibilityAction.get(), &QAction::toggled,
          [=](bool checked) { m_label->setVisible(checked); });

  m_label =
      new Label(this, QString::fromStdString(m_component->getDisplayName()),
                m_labelVisibilityAction);

  // Create IO ports of Component
  for (const auto &p_in : m_component->getInputPorts<SimPort>()) {
    m_inputPorts[p_in] =
        new PortGraphic(p_in, vsrtl::SimPort::PortType::in, this);
  }
  for (const auto &p_out : m_component->getOutputPorts<SimPort>()) {
    m_outputPorts[p_out] =
        new PortGraphic(p_out, vsrtl::SimPort::PortType::out, this);
  }

  m_restrictSubcomponentPositioning = false;
  if (hasSubcomponents()) {
    // Setup expand button
    m_expandButton = new ComponentButton(this);
    connect(m_expandButton, &ComponentButton::toggled,
            [this](bool expanded) { setExpanded(expanded); });

    createSubcomponents(doPlaceAndRoute);
    if (doPlaceAndRoute) {
      placeAndRouteSubcomponents();
    }
  }

  connect(this, &GridComponent::gridRectChanged, this,
          &ComponentGraphic::updateGeometry);
  connect(this, &GridComponent::portPosChanged, this,
          &ComponentGraphic::handlePortPosChanged);
  connect(this, &GridComponent::gridPosChanged, this,
          &ComponentGraphic::handleGridPosChange);

  // By default, a component is collapsed. This has no effect if a component
  // does not have any subcomponents
  setExpanded(false);
  m_restrictSubcomponentPositioning = true;

  updateGeometry();
  spreadPorts();
}

/**
 * @brief ComponentGraphic::createSubcomponents
 * In charge of hide()ing subcomponents if the parent component (this) is not
 * expanded
 */
void ComponentGraphic::createSubcomponents(bool doPlaceAndRoute) {
  for (const auto &c : m_component->getSubComponents()) {
    ComponentGraphic *nc;
    auto type = c->getGraphicsType();
    if (type == GraphicsTypeFor(Multiplexer)) {
      nc = new MultiplexerGraphic(c, this);
    } else if (type == GraphicsTypeFor(Constant)) {
      // Don't create a distinct ComponentGraphic for constants - these will be
      // drawn next to the port connecting to it
      continue;
    } else {
      nc = new ComponentGraphic(c, this);
    }
    nc->initialize(doPlaceAndRoute);
    nc->setParentItem(this);
    nc->setZValue(VSRTLScene::Z_Component);
    m_subcomponents.push_back(nc);
    if (!isExpanded()) {
      nc->hide();
    }
  }
}

void ComponentGraphic::resetWires() {
  const QString text = "Reset wires?\nThis will remove all interconnecting "
                       "points for all wires within this subcomponent";

  if (QMessageBox::Yes == QMessageBox::question(QApplication::activeWindow(),
                                                "Reset wires", text)) {
    // Clear subcomponent wires
    for (const auto &c : m_subcomponents) {
      for (const auto &p : c->outputPorts()) {
        p->getOutputWire()->clearWirePoints();
      }
    }
    // Clear wires from this components input ports
    for (const auto &p : std::as_const(m_inputPorts)) {
      p->getOutputWire()->clearWirePoints();
    }
  }
}

void ComponentGraphic::loadLayoutFile(const QString &fileName) {
  std::ifstream file(fileName.toStdString());
  cereal::JSONInputArchive archive(file);
  m_isTopLevelSerializedComponent = true;

  try {
    archive(CEREAL_NVP(m_layoutVersion));
  } catch (const cereal::Exception &e) {
    // No layout version
    m_layoutVersion = 0;
  }

  try {
    archive(cereal::make_nvp("ComponentGraphic", *this));
  } catch (const cereal::Exception &e) {
    /// @todo: build an error report
  }

  m_isTopLevelSerializedComponent = false;
  file.close();
}

void ComponentGraphic::loadLayout() {
  QString fileName = QFileDialog::getOpenFileName(
      QApplication::activeWindow(),
      "Save Layout " + QString::fromStdString(m_component->getName()),
      QString(), tr("JSON (*.json)"));

  if (fileName.isEmpty())
    return;

  loadLayoutFile(fileName);
}

void ComponentGraphic::saveLayout() {
  QString fileName = QFileDialog::getSaveFileName(
      QApplication::activeWindow(),
      "Save Layout " + QString::fromStdString(m_component->getName()),
      QString(), tr("JSON (*.json)"));

  if (fileName.isEmpty())
    return;
  if (!fileName.endsWith(".json"))
    fileName += ".json";
  std::ofstream file(fileName.toStdString());
  cereal::JSONOutputArchive archive(file);

  /// @todo: Is it more applicable to do a typeid(getComponent()).name() ? this
  /// would not work accross separate compilers, but would directly indicate the
  /// underlying type of which this layout is compatible with...
  m_isTopLevelSerializedComponent = true;
  try {
    m_layoutVersion = LatestLayoutVersion - 1;
    archive(CEREAL_NVP(m_layoutVersion));
    archive(cereal::make_nvp("ComponentGraphic", *this));
  } catch (const cereal::Exception &e) {
    /// @todo: build an error report
  }
  m_isTopLevelSerializedComponent = false;
}

void ComponentGraphic::parameterDialogTriggered() {
  ParameterDialog dialog(m_component);

  if (dialog.exec()) {
  }
  return;
}

void ComponentGraphic::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
  QMenu menu;

  if (!m_component->getParameters().empty()) {
    auto *parameterAction = menu.addAction("Parameters");
    connect(parameterAction, &QAction::triggered, this,
            &ComponentGraphic::parameterDialogTriggered);
  }

  if (hasSubcomponents() && !isLocked()) {
    // ======================== Layout menu ============================ //
    auto *layoutMenu = menu.addMenu("Layout");
    auto *loadAction = layoutMenu->addAction("Load layout");
    auto *saveAction = layoutMenu->addAction("Save layout");
    auto *resetWiresAction = layoutMenu->addAction("Reset wires");

    connect(saveAction, &QAction::triggered, this,
            &ComponentGraphic::saveLayout);
    connect(loadAction, &QAction::triggered, this,
            &ComponentGraphic::loadLayout);
    connect(resetWiresAction, &QAction::triggered, this,
            &ComponentGraphic::resetWires);
  }

  if (m_outputPorts.size() > 0) {
    // ======================== Ports menu ============================ //
    auto *portMenu = menu.addMenu("Ports");
    auto *showOutputsAction = portMenu->addAction("Show output values");
    auto *hideOutputsAction = portMenu->addAction("Hide output values");
    connect(showOutputsAction, &QAction::triggered, [=] {
      for (auto &c : m_outputPorts)
        c->setValueLabelVisible(true);
    });
    connect(hideOutputsAction, &QAction::triggered, [=] {
      for (auto &c : m_outputPorts)
        c->setValueLabelVisible(false);
    });

    if (!isLocked()) {
      auto *hiddenPortsMenu = portMenu->addMenu("Hidden ports");
      for (const auto &p : m_component->getAllPorts()) {
        auto *gp = p->getGraphic<PortGraphic>();
        if (gp->userHidden()) {
          auto *showPortAction =
              hiddenPortsMenu->addAction(QString::fromStdString(p->getName()));
          connect(showPortAction, &QAction::triggered,
                  [=] { gp->setUserVisible(true); });
        }
      }
      if (hiddenPortsMenu->actions().size() == 0) {
        delete hiddenPortsMenu;
      }
    }
  }

  // ======================== Indicators menu ============================ //
  if ((m_inputPorts.size() > 0) && !isLocked()) {
    auto *indicatorMenu = menu.addMenu("Indicators");
    for (const auto &portmap : {m_inputPorts, m_outputPorts}) {
      for (const auto &p : portmap) {
        // Value indicators for boolean signals. Boolean indicators will be
        // visible even if the port responsible for the indicator gets hidden.
        if (p->getPort()->getWidth() == 1) {
          auto *indicatorAction = indicatorMenu->addAction(
              QString::fromStdString(p->getPort()->getName()));
          indicatorAction->setCheckable(true);
          indicatorAction->setChecked(m_indicators.count(p));
          connect(indicatorAction, &QAction::triggered,
                  [=](bool checked) { setIndicatorState(p, checked); });
        }
      }
    }
    if (indicatorMenu->actions().size() == 0) {
      delete indicatorMenu;
    }
  }

  // ======================== Rotation menu ============================ //
  if ((m_component->getParent() != nullptr) && !isLocked()) {
    auto *rotationMenu = menu.addMenu("Rotate");
    auto *rotateClockwiseAction = rotationMenu->addAction("+90º");
    auto *rotateCounterClockwiseAction = rotationMenu->addAction("-90º");
    connect(rotateClockwiseAction, &QAction::triggered,
            [=] { gridRotate(RotationDirection::RightHand); });
    connect(rotateCounterClockwiseAction, &QAction::triggered,
            [=] { gridRotate(RotationDirection::LeftHand); });
  }

  if ((m_component->getParent() != nullptr) && !isLocked()) {
    auto *hideAction = menu.addAction("Hide component");
    connect(hideAction, &QAction::triggered, [=] {
      m_userHidden = true;
      this->hide();
    });
  }

  if (!isLocked()) {
    menu.addAction(m_labelVisibilityAction.get());
  }

  menu.exec(event->screenPos());
}

void ComponentGraphic::setIndicatorState(PortGraphic *p, bool enabled) {
  if (enabled) {
    m_indicators.emplace(p);
  } else {
    m_indicators.erase(p);
  }
  update();
}

void ComponentGraphic::registerWire(WireGraphic *wire) {
  m_wires.push_back(wire);
}

void ComponentGraphic::setExpanded(bool state) {
  GridComponent::setExpanded(state);
  bool areWeExpanded = isExpanded();
  if (m_expandButton != nullptr) {
    m_expandButton->setChecked(areWeExpanded);
    for (const auto &c : m_subcomponents) {
      const bool visible = areWeExpanded && !c->userHidden();
      if (visible) {
        c->setParentItem(this);
      } else if (auto *scenep = scene()) {
        scenep->removeItem(c);
      }
      c->setVisible(visible);
    }
    // We are not hiding the input ports of a component, because these should
    // always be drawn. However, a input port of an expandable component has
    // wires drawin inside the component, which must be hidden aswell, such that
    // they do not accept mouse events nor are drawn.
    for (const auto &w : m_wires) {
      w->setVisible(areWeExpanded);
    }
  }
}

void ComponentGraphic::setUserVisible(bool visible) {
  m_userHidden = !visible;
  setVisible(visible);
}

ComponentGraphic *ComponentGraphic::getParent() const {
  return dynamic_cast<ComponentGraphic *>(parentItem());
}

void ComponentGraphic::updateGeometry() {
  prepareGeometryChange();
  const QRectF sceneRect = sceneGridRect();
  const QPointF sceneRectCenter = {sceneRect.width() / 2.0,
                                   sceneRect.height() / 2.0};
  const QRect &currentGridRect = getCurrentComponentRect();

  // Apply rotation around center of shape. All shape points are defined in grid
  // [x,y] in [0:1], so rotate around [0.5, 0.5] Next, separately apply the
  // scaling through a secondary matrix (The transformation gets a lot simpler
  // like this, rather than composing translation + rotation +translation +
  // scaling in a single matrix.
  QTransform t;
  QMatrix4x4 mat;
  mat.scale(sceneRect.width(), sceneRect.height());
  t.translate(0.5, 0.5).rotate(gridRotation()).translate(-0.5, -0.5);
  m_shape = mat.toTransform().map(
      ShapeRegister::getTypeShape(m_component->getGraphicsType(), t));

  // Position the expand-button
  if (hasSubcomponents()) {
    if (isExpanded()) {
      m_expandButton->setPos(QPointF(0, 0));
    } else {
      // Center
      const qreal x =
          sceneRectCenter.x() - m_expandButton->boundingRect().width() / 2;
      const qreal y =
          sceneRectCenter.y() - m_expandButton->boundingRect().height() / 2;
      m_expandButton->setPos(QPointF(x, y));
    }
  }

  // Update the grid points within this component, if it has subcomponents
  if (hasSubcomponents() && isExpanded()) {
    // Grid should only be drawing inside the component, so remove 1 gridsize
    // from each edge of the component rect
    auto rect = m_shape.boundingRect();
    QPoint gridTopLeft = (rect.topLeft() / GRID_SIZE).toPoint() * GRID_SIZE;
    gridTopLeft += QPoint(GRID_SIZE, GRID_SIZE);
    QPoint gridBotRight =
        (rect.bottomRight() / GRID_SIZE).toPoint() * GRID_SIZE;
    gridBotRight -= QPoint(GRID_SIZE, GRID_SIZE);

    m_gridPoints.clear();
    for (int x = gridTopLeft.x(); x <= gridBotRight.x(); x += GRID_SIZE)
      for (int y = gridTopLeft.y(); y <= gridBotRight.y(); y += GRID_SIZE)
        m_gridPoints << QPoint(x, y);
  }

  if (!isSerializing()) {
    // Adjust label position through scaling by the relative size change of the
    // component.
    const auto &lastComponentRect = getLastComponentRect();
    if (lastComponentRect != QRect()) {
      const auto widthScaledChanged =
          static_cast<qreal>(currentGridRect.width()) /
          lastComponentRect.width();
      const auto heightScaledChanged =
          static_cast<qreal>(currentGridRect.height()) /
          lastComponentRect.height();

      // Scale the positioning of the label, adjusting it accordingly to the
      // component size change
      auto labelPos = m_label->pos();
      labelPos.rx() *= widthScaledChanged;
      labelPos.ry() *= heightScaledChanged;
      m_label->setPos(labelPos);
    } else {
      // First time setting label position. Position label centered above
      // component.
      m_label->setPos((sceneRect.width() / 2) -
                          m_label->boundingRect().width() / 2,
                      -m_label->boundingRect().height());
    }
  }
}

bool ComponentGraphic::handlePortGraphicMoveAttempt(
    const PortGraphic *port, const QPointF &newBorderPos) {
  // Port will report its new position in its portGraphic coordinates. Transfor
  // to this (the port parent) coordinate system and attempt to adjust port
  // position.

  const QPoint adjustedPos =
      sceneToGrid(mapFromItem(port, newBorderPos).toPoint());
  return adjustPort(port->getPort(), adjustedPos);
}

void ComponentGraphic::handleGridPosChange(const QPoint p) {
  setPos(gridToScene(p));
}

void ComponentGraphic::handlePortPosChanged(const SimPort *port) {
  const auto pos = getPortPos(port);
  auto *g = port->getGraphic<PortGraphic>();

  g->setSide(pos.side);
  switch (pos.side) {
  case Side::Left: {
    g->setPos(QPointF(sceneGridRect().left(), pos.index * GRID_SIZE));
    break;
  }
  case Side::Right: {
    g->setPos(QPointF(sceneGridRect().right(), pos.index * GRID_SIZE));
    break;
  }
  case Side::Top: {
    g->setPos(QPointF(pos.index * GRID_SIZE, sceneGridRect().top()));
    break;
  }
  case Side::Bottom: {
    g->setPos(QPointF(pos.index * GRID_SIZE, sceneGridRect().bottom()));
    break;
  }
  }
}

void ComponentGraphic::setLocked(bool locked) {
  // No longer give the option of expand the component if the scene is locked
  if (m_expandButton)
    m_expandButton->setVisible(!locked);

  GraphicsBaseItem::setLocked(locked);
}

QVariant ComponentGraphic::itemChange(QGraphicsItem::GraphicsItemChange change,
                                      const QVariant &value) {
  Q_ASSERT((flags() & QGraphicsItem::ItemSendsGeometryChanges) &&
           "Need ItemSendsGeometryChanges for ItemPositionHasChanged");

  // @todo implement snapping inside parent component
  if (change == ItemPositionChange && scene()) {
    // Output port wires are implicitely redrawn given that the wire is a child
    // of $this. We need to manually signal the wires going to the input ports
    // of this component, to redraw
    if (m_initialized) {
      for (const auto &inputPort : std::as_const(m_inputPorts)) {
        if (!inputPort->getPort()->isConstant()) {
          if (auto *simInputPort = inputPort->getPort()->getInputPort()) {
            if (auto *graphic = simInputPort->getGraphic<PortGraphic>()) {
              graphic->updateWireGeometry();
            }
          }
        }
      }
    }

    if (parentIsPlacing()) {
      // New position has been validated (and set) through the grid layer, and
      // the grid component has >already< been moved to its new position. This
      // slot is called through the move signal of the gridComponent, emitted
      // during place and route.
      return value;
    } else {
      /// Parent is not placing, @p value represents a QPointF in parent scene
      /// coordinates. Go through GridComponent to validate and attempt to place
      /// the component.
      const QPoint newGridPos = sceneToGrid(value.toPoint());

      if (move(newGridPos)) {
        return gridToScene(getGridPos());
      } else {
        // Move was unsuccessfull, keep current positioning
        return pos();
      }
    }
  }

  if (change == ItemPositionHasChanged) {
    // Notify ports that their position inside the module has changed
    for (const auto &portmap : {m_inputPorts, m_outputPorts}) {
      for (const auto &p : portmap) {
        p->modulePositionHasChanged();
      }
    }
  }

  return GraphicsBaseItem::itemChange(change, value);
}

void ComponentGraphic::paint(QPainter *painter,
                             const QStyleOptionGraphicsItem *option,
                             QWidget *w) {
  painter->save();
  QColor color;
  if (static_cast<VSRTLScene *>(scene())->darkmode()) {
    color = hasSubcomponents() && isExpanded()
                ? QColorConstants::DarkGray.darker()
                : QColor{0x80, 0x84, 0x8a};
  } else {
    color = hasSubcomponents() && isExpanded() ? QColor{0xec, 0xf0, 0xf1}
                                               : QColorConstants::White;
  }

  QColor fillColor =
      (option->state & QStyle::State_Selected) ? color.darker(150) : color;
  if (option->state & QStyle::State_MouseOver)
    fillColor = fillColor.lighter(125);

  const qreal lod =
      option->levelOfDetailFromTransform(painter->worldTransform());

  // Draw component outline
  QPen oldPen = painter->pen();
  QPen pen = oldPen;
  int width = COMPONENT_BORDER_WIDTH;
  if (option->state & QStyle::State_Selected)
    width += 1;

  pen.setWidth(width);
  painter->setBrush(QBrush(
      fillColor.darker((option->state & QStyle::State_Sunken) ? 120 : 100)));
  painter->setPen(pen);
  painter->drawPath(m_shape);

  painter->setPen(oldPen);

  if (hasSubcomponents()) {
    if (lod >= 0.35) {
      // Determine whether expand button should be shown. If we are in locked
      // state, do not interfere with the view state of the expand button
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
  for (const auto &p : m_indicators) {
    paintIndicator(painter, p, p->getPort()->uValue() ? Qt::green : Qt::red);
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

void ComponentGraphic::paintIndicator(QPainter *painter, PortGraphic *p,
                                      QColor color) {
  painter->save();
  constexpr qreal dotSize = 12;
  QPen pen = painter->pen();
  pen.setWidth(WIRE_WIDTH - 1);
  painter->setBrush(color);
  painter->setPen(pen);

  const bool inPort = p->getPortType() == vsrtl::SimPort::PortType::in;
  QRectF chordRect(-dotSize / 2, -dotSize / 2, dotSize, dotSize);
  chordRect.translate(
      mapFromItem(p, inPort ? p->getOutputPoint() : p->getInputPoint()));

  int startAngle = 0;
  // clang-format off
    switch(p->getSide()){
        case Side::Top : startAngle = 0; break;
        case Side::Bottom : startAngle = -180; break;
        case Side::Left : startAngle = 90; break;
        case Side::Right : startAngle = -90; break;
    }
  // clang-format on

  painter->drawChord(chordRect, startAngle * 16, -180 * 16);
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

void ComponentGraphic::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  if (flags().testFlag(ItemIsMovable) && event->button() == Qt::LeftButton &&
      m_inResizeDragZone) {
    // start resize drag
    setFlags(flags() & ~ItemIsMovable);
    m_resizeDragging = true;
  }

  QGraphicsItem::mousePressEvent(event);
}

void ComponentGraphic::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if (m_resizeDragging) {
    QPoint gridPos = (event->pos() / GRID_SIZE).toPoint();
    const auto oldGridRect = getCurrentComponentRect();
    auto newGridRect = oldGridRect;
    newGridRect.setBottomRight(gridPos);

    adjust(newGridRect);
  }

  QGraphicsItem::mouseMoveEvent(event);
}

void ComponentGraphic::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (m_resizeDragging) {
    setFlags(flags() | ItemIsMovable);
    m_resizeDragging = false;
  }

  QGraphicsItem::mouseReleaseEvent(event);
}

void ComponentGraphic::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
  if (!isLocked()) {
    const auto &sceneRect = sceneGridRect();
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
} // namespace vsrtl
