#include "vsrtl_portgraphic.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_port.h"
#include "vsrtl_wiregraphic.h"

#include "math.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

int PortGraphic::s_portGridWidth = 2;

PortGraphic::PortGraphic(SimPort* port, PortType type, QGraphicsItem* parent)
    : m_port(port), m_type(type), GraphicsBase(parent) {
    port->registerGraphic(this);
    m_widthText = QString::number(port->getWidth() - 1) + ":0";
    m_font = QFont("Monospace", 8);
    m_pen.setWidth(WIRE_WIDTH);
    m_pen.setCapStyle(Qt::RoundCap);
    setAcceptHoverEvents(true);

    if (m_port->isEnumPort()) {
        // By default, display Enum value if underlying port is enum
        m_Radix = Radix::Enum;
    }
    m_valueLabel = new ValueLabel(m_Radix, m_port, this);
    m_valueLabel->setVisible(false);
    m_valueLabel->moveBy(0, -10);  // start position (may be dragged)

    port->changed.Connect(this, &PortGraphic::updateSlot);

    m_colorAnimation = new QPropertyAnimation(this, "penColor");
    m_colorAnimation->setDuration(100);
    m_colorAnimation->setStartValue(WIRE_BOOLHIGH_COLOR);
    m_colorAnimation->setEndValue(WIRE_DEFAULT_COLOR);
    m_colorAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(m_colorAnimation, &QPropertyAnimation::valueChanged, this, &PortGraphic::updatePenColor);

    setFlag(ItemIsSelectable);

    updateGeometry();

    m_portPoint = new PortPoint(this);
    m_portPoint->setPos(mapToItem(this, mapToScene(type == PortType::in ? getInputPoint() : getOutputPoint())));

    m_outputWire = new WireGraphic(this, m_port->getOutputPorts(), this);
}

void PortGraphic::setOutwireVisible(bool state) {
    m_outputWire->setVisible(state);
}

void PortGraphic::updateSlot() {
    updatePen();
    update();

    // Propagate any changes to current port value to this label, and all other connected ports which may have their
    // labels visible
    m_valueLabel->updateText();
}

void PortGraphic::setLabelVisible(bool visible) {
    m_valueLabel->setVisible(visible);
    updateSlot();
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

void PortGraphic::propagateRedraw() {
    m_port->traverseToSinks([=](SimPort* port) {
        auto* portGraphic = port->getGraphic<PortGraphic>();
        portGraphic->redraw();
    });
}

/**
 * @brief PortGraphic::postSceneConstructionInitialize2
 * With all wires created, the port may now issue an updatePen() call. This must be executed after wires have
 * initialized, given that updatePen() will trigger a redraw of wires
 */
void PortGraphic::postSceneConstructionInitialize2() {
    if (!m_inputWire)
        updatePen();

    if (m_port->isConstant()) {
        // For constant ports, we by default display the value of the port
        m_valueLabel->show();
        m_Radix = Radix::Signed;
        // Update the ValueLabel (Letting it resize to its final value) and position it next to the port
        updateSlot();
        const auto br = m_valueLabel->boundingRect();
        m_valueLabel->setPos({-br.width() + 5, (br.height() + br.y()) / 4});

        // Initial port color is implicitely set by triggering the wire animation
        m_colorAnimation->start();
    }
}

void PortGraphic::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    menu.addMenu(createPortRadixMenu(m_port, m_Radix));

    if (!isLocked()) {
        QAction* showLabel = new QAction("Show value");
        showLabel->setCheckable(true);
        showLabel->setChecked(m_valueLabel->isVisible());
        connect(showLabel, &QAction::triggered, [this](bool checked) {
            setLabelVisible(checked);
            updateSlot();
        });
        menu.addAction(showLabel);
    }

    menu.exec(event->screenPos());
    m_valueLabel->updateText();
}

void PortGraphic::setInputWire(WireGraphic* wire) {
    // Set wire is called during post scene construction initialization, wherein WireGraphic's will register with its
    // destination ports (inputs)
    Q_ASSERT(m_inputWire == nullptr);
    m_inputWire = wire;
}

QRectF PortGraphic::boundingRect() const {
    return m_boundingRect;
}

QPointF PortGraphic::getInputPoint() const {
    return QPointF(0, 0);
}

QPointF PortGraphic::getOutputPoint() const {
    return QPointF(s_portGridWidth * GRID_SIZE, 0);
}

void PortGraphic::updatePenColor() {
    // This is a source port. Update pen based on current state
    // Selection check is based on whether item is currently selected or about to be selected (via itemChange())
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
    m_port->traverseToRoot([=](SimPort* node) {
        if (node->isConstant()) {
            return;
        }

        // Traverse to root, and only execute when no input wire is present. This signifies that the root source
        // port has been reached
        auto* portGraphic = node->getGraphic<PortGraphic>();
        if (!portGraphic->m_inputWire) {
            if (aboutToBeDeselected || aboutToBeSelected) {
                portGraphic->m_signalSelected = aboutToBeSelected;
            }

            /* If the port is anything other than a boolean port, a change in the signal is represented by starting
             * the color change animation. If it is a boolean signal, just update the pen color */
            if (m_port->getWidth() != 1) {
                portGraphic->m_colorAnimation->start();
            } else {
                portGraphic->updatePenColor();
            }

            // Make output port cascade an update call to all ports and wires which originate from this source
            portGraphic->propagateRedraw();
        }
    });
}

QString PortGraphic::getTooltipString() const {
    return QString::fromStdString(m_port->getDisplayName() + ":\n") + encodePortRadixValue(m_port, m_Radix);
}

QVariant PortGraphic::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemSelectedChange) {
        updatePen(value.toBool(), !value.toBool());
    }

    if (change == QGraphicsItem::ItemPositionChange) {
        QPoint dPos = value.toPoint() - pos().toPoint();
        if (m_type == PortType::out) {
            m_outputWire->portMoved(this, dPos);
        } else {
            m_inputWire->portMoved(this, dPos);
        }
    }

    if (m_inputWire && m_type == PortType::in && change == QGraphicsItem::ItemVisibleChange) {
        // Inform wires terminating in this port to set their visibility based on this ports visibility
        m_inputWire->setWiresVisibleToPort(m_portPoint, value.toBool());
    }

    return QGraphicsItem::itemChange(change, value);
}

void PortGraphic::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    // Move port position
    if (isSelected()) {
        auto* parent = static_cast<ComponentGraphic*>(parentItem());
        if (parent->handlePortGraphicMoveAttempt(this, event->pos())) {
            update();
        }
    }
}

void PortGraphic::hoverEnterEvent(QGraphicsSceneHoverEvent* event) {
    m_hoverActive = true;
    update();
}

void PortGraphic::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    m_hoverActive = false;
    update();
}

void PortGraphic::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    setToolTip(getTooltipString());
}

void PortGraphic::updateGeometry() {
    QFontMetrics fm(m_font);
    m_textRect = fm.boundingRect(m_widthText);

    m_boundingRect = QRectF(getInputPoint(), m_textRect.size());
    const qreal portLen = (getOutputPoint() - getInputPoint()).manhattanLength();
    m_boundingRect.setWidth(m_boundingRect.width() < portLen ? portLen : m_boundingRect.width());

    // Adjust for pen sizes etc. (set via. visual inspection of the bounding rect)
    m_boundingRect.adjust(-3, -3, 3, 1);
}

const QPen& PortGraphic::getPen() {
    // Only source ports (ports with no input wires) can provide a pen.
    // Sink ports request their pens from their endpoint source port. This call might go through multiple port
    // in/out combinations and component boundaries before reaching the pen.
    if (m_inputWire) {
        return m_inputWire->getFromPort()->getPen();
    } else {
        return m_pen;
    }
}

void PortGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    painter->save();
    painter->setFont(m_font);
    const int offset = m_type == PortType::out ? PORT_INNER_MARGIN
                                               : s_portGridWidth * GRID_SIZE - m_textRect.width() - PORT_INNER_MARGIN;

    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());
    if (lod >= 0.6) {
        painter->drawText(QPointF(offset, m_textRect.height() / 2 + PORT_INNER_MARGIN), m_widthText);
    }

    painter->setPen(getPen());
    painter->drawLine(getInputPoint(), getOutputPoint());

    if (m_hoverActive) {
        // Draw an arrow indicating the direction of the port
        const auto d = std::sqrt(std::pow(GRID_SIZE / 2, 2) + std::pow(GRID_SIZE / 2, 2)) / 2;
        const QPointF start = (getOutputPoint() - getInputPoint()) / 2;
        const QPointF p1 = start - QPointF(d, d);
        const QPointF p2 = start - QPointF(d, -d);
        painter->drawLine(start, p1);
        painter->drawLine(start, p2);
    }

    painter->restore();

#ifdef VSRTL_DEBUG_DRAW
    DRAW_BOUNDING_RECT(painter)
#endif
}

}  // namespace vsrtl
