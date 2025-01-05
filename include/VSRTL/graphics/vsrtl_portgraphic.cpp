#include "vsrtl_portgraphic.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_port.h"
#include "vsrtl_scene.h"
#include "vsrtl_wiregraphic.h"

#include "math.h"

#include <QAction>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

constexpr int s_portGridWidth = 1;

PortGraphic::PortGraphic(SimPort *port, vsrtl::SimPort::PortType type,
                         QGraphicsItem *parent)
    : GraphicsBaseItem(parent), m_type(type), m_port(port) {
  port->registerGraphic(this);
  m_font = QFont("Monospace", 8);
  m_pen.setWidth(WIRE_WIDTH);
  m_pen.setCapStyle(Qt::RoundCap);
  setAcceptHoverEvents(true);
  ComponentGraphic *directParent =
      static_cast<ComponentGraphic *>(parentItem());

  m_radix = std::make_shared<Radix>(Radix::Hex);
  if (m_port->isEnumPort()) {
    // By default, display Enum value if underlying port is enum
    *m_radix = Radix::Enum;
  }

  // Connect changes from simulator through our signal translation mechanism
  wrapSimSignal(port->changed);

  m_colorAnimation = std::make_unique<QPropertyAnimation>(this, "penColor");
  m_colorAnimation->setDuration(100);
  m_colorAnimation->setStartValue(m_port->getWidth() == 1 ? WIRE_BOOLHIGH_COLOR
                                                          : WIRE_HIGH_COLOR);

  m_colorAnimation->setEndValue(WIRE_DEFAULT_COLOR);
  m_colorAnimation->setEasingCurve(QEasingCurve::Linear);
  connect(m_colorAnimation.get(), &QPropertyAnimation::valueChanged, this,
          &PortGraphic::updatePenColor);

  setFlag(ItemIsSelectable);

  m_inputPortPoint = new PortPoint(this);
  m_inputPortPoint->setPos(getInputPoint());
  m_outputPortPoint = new PortPoint(this);
  m_outputPortPoint->setPos(getOutputPoint());

  ComponentGraphic *scopeParent = nullptr;

  if (m_type == vsrtl::SimPort::PortType::in) {
    scopeParent = directParent;
    m_outputWire = new WireGraphic(scopeParent, this, m_port->getOutputPorts(),
                                   WireGraphic::WireType::BorderOutput);
  } else {
    // Output wires exists in the domain in between different components in the
    // PARENT of the parent of this port.
    scopeParent = dynamic_cast<ComponentGraphic *>(moduleParent());
    m_outputWire = new WireGraphic(scopeParent, this, m_port->getOutputPorts(),
                                   WireGraphic::WireType::ComponentOutput);
  }
  m_outputWire->setZValue(VSRTLScene::Z_Wires);

  // Setup actions
  m_showValueAction = std::make_shared<QAction>("Show value");
  m_showValueAction->setCheckable(true);
  m_showValueAction->setChecked(false);
  connect(m_showValueAction.get(), &QAction::toggled, this,
          &PortGraphic::setValueLabelVisible);

  m_showWidthAction = std::make_shared<QAction>("Show width");
  m_showWidthAction->setCheckable(true);
  m_showWidthAction->setChecked(true);
  connect(m_showWidthAction.get(), &QAction::toggled, this,
          &PortGraphic::setPortWidthVisible);

  m_showLabelAction = std::make_shared<QAction>("Show label");
  m_showLabelAction->setCheckable(true);
  m_showLabelAction->setChecked(true);
  connect(m_showLabelAction.get(), &QAction::toggled, [this](bool checked) {
    m_label->setVisible(checked);
    m_label->setLocked(false);
  });

  m_showAction = std::make_shared<QAction>("Show port");
  m_showAction->setCheckable(true);
  connect(m_showAction.get(), &QAction::toggled, this,
          &PortGraphic::setUserVisible);

  // Setup labels
  m_label =
      new Label(directParent, QString::fromStdString(m_port->getDisplayName()),
                m_showLabelAction, 8);
  addVirtualChild({VirtualChildLink::Visibility, VirtualChildLink::Position},
                  m_label);
  m_label->setVisible(false);
  m_label->setZValue(VSRTLScene::Z_PortLabel);

  m_valueLabel = createModuleChild<ValueLabel>(
      {VirtualChildLink::Visibility, VirtualChildLink::Position}, m_radix, this,
      m_showValueAction);
  m_valueLabel->setVisible(false);
  directParent->addVirtualChild(
      VirtualChildLink::Position,
      m_valueLabel); // Also move when direct parent moves
  m_valueLabel->setZValue(VSRTLScene::Z_ValueLabel);

  m_portWidthLabel =
      new Label(directParent, QString::number(port->getWidth() - 1) + ":0",
                m_showWidthAction, 7);
  addVirtualChild({VirtualChildLink::Visibility, VirtualChildLink::Position},
                  m_portWidthLabel);
  m_portWidthLabel->setMoveable(false);
  m_portWidthLabel->setHoverable(false);
  m_portWidthLabel->setFlags(
      m_portWidthLabel->flags() &
      ~(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable));
  m_portWidthLabel->setZValue(VSRTLScene::Z_PortWidth);

  updateGeometry();
}

void PortGraphic::simUpdateSlot() {
  Q_ASSERT(m_valueLabel);
  updatePen();
  update();

  // Propagate any changes to current port value to this label, and all other
  // connected ports which may have their labels visible
  m_valueLabel->updateText();
}

void PortGraphic::setValueLabelVisible(bool visible) {
  Q_ASSERT(m_valueLabel);
  if (!userHidden() || m_valueLabel->isVisible()) {
    m_showValueAction->setChecked(visible);
    m_valueLabel->setVisible(visible);
    simUpdateSlot();
  }
}

void PortGraphic::setPortWidthVisible(bool visible) {
  Q_ASSERT(m_portWidthLabel);
  m_portWidthLabel->setVisible(visible);
  update();
}

void PortGraphic::updateWireGeometry() {
  m_outputWire->prepareGeometryChange();
}

void PortGraphic::redraw() {
  // Schedules redrawing of the current port and its output wire
  update();
  if (m_outputWire) {
    m_outputWire->update();
  }
}

void PortGraphic::setSide(Side side) {
  m_side = side;
  updateGeometry();
}

void PortGraphic::propagateRedraw() {
  m_port->traverseToSinks([=](SimPort *port) {
    if (auto *portGraphic = port->getGraphic<PortGraphic>()) {
      portGraphic->redraw();
    }
  });
}

/**
 * @brief PortGraphic::postSceneConstructionInitialize2
 * With all wires created, the port may now issue an updatePen() call. This must
 * be executed after wires have initialized, given that updatePen() will trigger
 * a redraw of wires
 */
void PortGraphic::postSceneConstructionInitialize2() {
  if (!m_inputWire) {
    updatePen();
  }

  if (m_port->isConstant()) {
    // For constant ports, we by default display the value of the port
    m_valueLabel->show();
    *m_radix = m_port->getWidth() == 1 ? Radix::Unsigned : Radix::Signed;
    simUpdateSlot(); // propagate radix change to value label

    const auto br = m_valueLabel->boundingRect();
    m_valueLabel->setPos(
        mapToItem(m_valueLabel->parentItem(),
                  {-br.width() - boundingRect().width(), -br.height() / 2}));

    // Initial port color is implicitely set by triggering the wire animation
    m_colorAnimation->start(QPropertyAnimation::KeepWhenStopped);
  }
}

void PortGraphic::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
  QMenu menu;

  if (!userHidden()) {
    menu.addAction(m_showValueAction.get());
  }

  if (!isLocked()) {
    menu.addAction(m_showWidthAction.get());
    menu.addAction(m_showLabelAction.get());
    m_showAction->setChecked(!userHidden());
    menu.addAction(m_showAction.get());
  }

  menu.exec(event->screenPos());
  m_valueLabel->updateText();
}

void PortGraphic::modulePositionHasChanged() {
  // Notify in- and output points that they have been moved within the module
  m_inputPortPoint->modulePositionHasChanged();
  m_outputPortPoint->modulePositionHasChanged();
}

void PortGraphic::setInputWire(WireGraphic *wire) {
  // Set wire is called during post scene construction initialization, wherein
  // WireGraphic's will register with its destination ports (inputs)
  Q_ASSERT(m_inputWire == nullptr);
  m_inputWire = wire;
}

QRectF PortGraphic::boundingRect() const { return m_boundingRect; }

QPointF PortGraphic::getInputPoint() const {
  switch (m_side) {
  case Side::Right: {
    return QPointF(m_type == vsrtl::SimPort::PortType::in
                       ? s_portGridWidth * GRID_SIZE
                       : 0,
                   0);
  }
  case Side::Left: {
    return QPointF(m_type == vsrtl::SimPort::PortType::in
                       ? -s_portGridWidth * GRID_SIZE
                       : 0,
                   0);
  }
  case Side::Top: {
    return QPointF(0, m_type == vsrtl::SimPort::PortType::in
                          ? -s_portGridWidth * GRID_SIZE
                          : 0);
  }
  case Side::Bottom: {
    return QPointF(0, m_type == vsrtl::SimPort::PortType::in
                          ? s_portGridWidth * GRID_SIZE
                          : 0);
  }
  }
  Q_UNREACHABLE();
}

QPointF PortGraphic::getOutputPoint() const {
  switch (m_side) {
  case Side::Right: {
    return QPointF(m_type == vsrtl::SimPort::PortType::out
                       ? s_portGridWidth * GRID_SIZE
                       : 0,
                   0);
  }
  case Side::Left: {
    return QPointF(m_type == vsrtl::SimPort::PortType::out
                       ? -s_portGridWidth * GRID_SIZE
                       : 0,
                   0);
  }
  case Side::Top: {
    return QPointF(0, m_type == vsrtl::SimPort::PortType::out
                          ? -s_portGridWidth * GRID_SIZE
                          : 0);
  }
  case Side::Bottom: {
    return QPointF(0, m_type == vsrtl::SimPort::PortType::out
                          ? s_portGridWidth * GRID_SIZE
                          : 0);
  }
  }
  Q_UNREACHABLE();
}

void PortGraphic::updatePenColor() {
  // This is a source port. Update pen based on current state
  // Selection check is based on whether item is currently selected or about to
  // be selected (via itemChange())
  if (m_signalSelected) {
    m_pen.setColor(WIRE_SELECTED_COLOR);
    m_pen.setWidth(static_cast<int>(WIRE_WIDTH * 1.5));
  } else {
    m_pen.setWidth(WIRE_WIDTH);
    if (m_port->getWidth() == 1) {
      if (static_cast<bool>(m_port->uValue())) {
        m_pen.setColor(WIRE_BOOLHIGH_COLOR);
      } else {
        m_pen.setColor(WIRE_DEFAULT_COLOR);
      }
    } else {
      m_pen.setColor(m_penColor);
    }
  }
  propagateRedraw();
}

void PortGraphic::updatePen(bool aboutToBeSelected, bool aboutToBeDeselected) {
  m_port->traverseToRoot([=](SimPort *node) {
    if (node->isConstant()) {
      return;
    }

    // Traverse to root, and only execute when no input wire is present. This
    // signifies that the root source port has been reached
    auto *portGraphic = node->getGraphic<PortGraphic>();
    if (portGraphic && !portGraphic->m_inputWire) {
      if (aboutToBeDeselected || aboutToBeSelected) {
        portGraphic->m_signalSelected = aboutToBeSelected;
      }

      /* If the port is anything other than a boolean port, a change in the
       * signal is represented by starting
       * the color change animation. If it is a boolean signal, just update the
       * pen color */
      if (m_port->getWidth() != 1) {
        portGraphic->m_colorAnimation->start(
            QPropertyAnimation::KeepWhenStopped);
      } else {
        portGraphic->updatePenColor();
      }

      // Make output port cascade an update call to all ports and wires which
      // originate from this source
      portGraphic->propagateRedraw();
    }
  });
}

QString PortGraphic::getTooltipString() const {
  return QString::fromStdString(m_port->getDisplayName() + ":\n") +
         encodePortRadixValue(m_port, *m_radix);
}

QVariant PortGraphic::itemChange(GraphicsItemChange change,
                                 const QVariant &value) {
  if (change == QGraphicsItem::ItemSelectedChange) {
    updatePen(value.toBool(), !value.toBool());
  }

  if (change == QGraphicsItem::ItemVisibleChange) {
    setPortVisible(value.toBool());
  }

  if (change == QGraphicsItem::ItemPositionHasChanged) {
    modulePositionHasChanged();
  }

  return GraphicsBaseItem::itemChange(change, value);
}

void PortGraphic::setPortVisible(bool visible) {
  m_inputPortPoint->setVisible(visible);
  m_outputPortPoint->setVisible(visible);
  if (m_inputWire && m_type == vsrtl::SimPort::PortType::in) {
    // Inform wires terminating in this port to set their visibility based on
    // this ports visibility
    m_inputWire->setWiresVisibleToPort(
        m_inputPortPoint, visible && m_sourceVisible && !m_userHidden);
  }

  else if (m_type == vsrtl::SimPort::PortType::out) {
    // hide all input ports of other components which this port is the source
    // of.
    for (const auto &p_conn : m_port->getOutputPorts()) {
      auto *portParent = p_conn->getParent();
      auto *portGraphic = p_conn->getGraphic<PortGraphic>();
      if (!portGraphic) {
        continue;
      }

      const bool isNestedComponent =
          portParent ==
          m_port->getParent<SimComponent>()->getParent<SimComponent>();

      if (!isNestedComponent && portParent && portGraphic) {
        portGraphic->setSourceVisible(visible && !m_userHidden);
      }
    }
  }
}

void PortGraphic::setSourceVisible(bool visible) {
  m_sourceVisible = visible;
  setPortVisible(visible);
  update();
}

void PortGraphic::setUserVisible(bool visible) {
  // User visibility only affects port draw state, >not< its scene visibility.
  m_userHidden = !visible;
  setPortVisible(visible);

  update();
}

void PortGraphic::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  // Move port position
  if (isSelected() && !isLocked()) {
    auto *parent = static_cast<ComponentGraphic *>(parentItem());
    if (parent->handlePortGraphicMoveAttempt(this, event->pos())) {
      update();
    }
  }
}

void PortGraphic::hoverEnterEvent(QGraphicsSceneHoverEvent *) {
  m_hoverActive = true;
  update();
}

void PortGraphic::hoverLeaveEvent(QGraphicsSceneHoverEvent *) {
  m_hoverActive = false;
  update();
}

void PortGraphic::hoverMoveEvent(QGraphicsSceneHoverEvent *) {
  setToolTip(getTooltipString());
}

void PortGraphic::updateGeometry() {
  prepareGeometryChange();

  m_boundingRect.setTopLeft({0, 0});

  // Note: When changing m_portWidthLabel positions, this is done with respect
  // to its parent (m_portWidthLabel is only a virtual child of this port).
  QPointF pDiff;
  switch (m_side) {
  case Side::Left:
  case Side::Right: {
    m_boundingRect.setY(-GRID_SIZE / 2);
    m_boundingRect.setHeight(GRID_SIZE);
    m_boundingRect.setX(m_side == Side::Left ? -GRID_SIZE * s_portGridWidth
                                             : 0);
    m_boundingRect.setWidth(GRID_SIZE * s_portGridWidth);

    const qreal vDiff =
        std::abs(GRID_SIZE / 2 - m_portWidthLabel->boundingRect().height() / 2);
    pDiff = m_side == Side::Left
                ? QPointF{-m_portWidthLabel->boundingRect().width(), -vDiff}
                : QPointF{0, -vDiff};
    break;
  }
  case Side::Top:
  case Side::Bottom: {
    m_boundingRect.setX(-GRID_SIZE / 2);
    m_boundingRect.setWidth(GRID_SIZE);
    m_boundingRect.setY(m_side == Side::Top ? -GRID_SIZE * s_portGridWidth : 0);
    m_boundingRect.setHeight(GRID_SIZE * s_portGridWidth);

    const qreal vDiff =
        std::abs(GRID_SIZE / 2 - m_portWidthLabel->boundingRect().height() / 2);
    pDiff =
        (m_side == Side::Top
             ? QPointF{0, -m_portWidthLabel->boundingRect().height() + vDiff}
             : QPointF{0, -vDiff});
    break;
  }
  }
  m_portWidthLabel->setPos(mapToItem(m_portWidthLabel->parentItem(), pDiff));

  // The shape of the component is defined as the above created rectangle
  m_shape = QPainterPath();
  m_shape.addPolygon(m_boundingRect);

  // Exapnd the rectangle to adjust for pen sizes etc.
  m_boundingRect.adjust(-WIRE_WIDTH, -WIRE_WIDTH, WIRE_WIDTH, WIRE_WIDTH);
}

QPainterPath PortGraphic::shape() const { return m_shape; }

const QPen &PortGraphic::getPen() {
  // Only source ports (ports with no input wires) can provide a pen.
  // Sink ports request their pens from their endpoint source port. This call
  // might go through multiple port in/out combinations and component boundaries
  // before reaching the pen.
  if (m_inputWire) {
    return m_inputWire->getFromPort()->getPen();
  } else {
    return m_pen;
  }
}

void PortGraphic::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                        QWidget *) {
  // Only draw the port if the source of the port is visible, or if the user is
  // currently hovering over the port.
  if (!((m_sourceVisible && !m_userHidden) || m_hoverActive))
    return;

  painter->save();
  painter->setPen(getPen());
  const QLineF portLine = QLineF(getInputPoint(), getOutputPoint());
  painter->drawLine(portLine);

  if (m_hoverActive) {
    // Draw an arrow indicating the direction of the port
    const auto d =
        std::sqrt(std::pow(GRID_SIZE / 2, 2) + std::pow(GRID_SIZE / 2, 2)) / 2;
    QPointF start = portLine.center();
    QPointF p1, p2;

    switch (m_side) {
    case Side::Left:
    case Side::Right: {
      int dir = m_type == vsrtl::SimPort::PortType::out ? -1 : 1;
      dir *= m_side == Side::Right ? -1 : 1;

      start.rx() = start.x() + dir * d / 2;

      p1 = start - QPointF(dir * d, d);
      p2 = start - QPointF(dir * d, -d);
      break;
    }

    case Side::Bottom:
    case Side::Top: {
      int dir = m_type == vsrtl::SimPort::PortType::in ? -1 : 1;
      dir *= m_side == Side::Top ? -1 : 1;

      start.ry() = start.y() + dir * d / 2;

      p1 = start - QPointF(d, dir * d);
      p2 = start - QPointF(-d, dir * d);
      break;
    }
    }
    painter->drawLine(start, p1);
    painter->drawLine(start, p2);
  }

  painter->restore();

#ifdef VSRTL_DEBUG_DRAW
  DRAW_BOUNDING_RECT(painter)
#endif
}

} // namespace vsrtl
