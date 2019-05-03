#include "vsrtl_portgraphic.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_port.h"
#include "vsrtl_traversal_util.h"
#include "vsrtl_wiregraphic.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

int PortGraphic::s_portGridWidth = 2;

PortGraphic::PortGraphic(Port* port, PortType type, QGraphicsItem* parent) : m_port(port), m_type(type) {
    port->registerGraphic(this);
    setParentItem(parent);
    m_widthText = QString::number(port->getWidth() - 1) + ":0";
    m_font = QFont("Monospace", 8);
    m_pen.setWidth(WIRE_WIDTH);
    m_pen.setCapStyle(Qt::RoundCap);

    // PortGraphic logic is build by revolving around the root source port in a port-wire connection. Only receive
    // update signals from root sources
    if (port->getInputPort() == nullptr) {
        port->changed.Connect(this, &PortGraphic::updateSlot);
    }
    m_colorAnimation = new QPropertyAnimation(this, "penColor");
    m_colorAnimation->setDuration(100);
    m_colorAnimation->setStartValue(WIRE_BOOLHIGH_COLOR);
    m_colorAnimation->setEndValue(WIRE_DEFAULT_COLOR);
    m_colorAnimation->setEasingCurve(QEasingCurve::Linear);
    connect(m_colorAnimation, &QPropertyAnimation::valueChanged, this, &PortGraphic::updatePenColor);

    setFlag(ItemIsSelectable);

    updateGeometry();

    m_outputWire = new WireGraphic(this, m_port->getOutputPorts(), this);
}

void PortGraphic::updateSlot() {
    updatePen();
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

void PortGraphic::setNet(pr::NetPtr& net) {
    m_outputWire->setNet(net);
}

void PortGraphic::propagateRedraw() {
    m_port->traverseToSinks([=](Port* port) {
        auto* portGraphic = getGraphic<PortGraphic*>(port);
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
    if (m_selected) {
        m_pen.setColor(WIRE_SELECTED_COLOR);
    } else if (m_port->getWidth() == 1) {
        if (static_cast<bool>(*m_port)) {
            m_pen.setColor(WIRE_BOOLHIGH_COLOR);
        } else {
            m_pen.setColor(WIRE_DEFAULT_COLOR);
        }
    } else {
        m_pen.setColor(m_penColor);
    }
    propagateRedraw();
}

void PortGraphic::updatePen(bool aboutToBeSelected, bool aboutToBeDeselected) {
    m_port->traverseToRoot([=](Port* node) {
        // Traverse to root, and only execute when no input wire is present. This signifies that the root source port
        // has been reached
        auto* portGraphic = getGraphic<PortGraphic*>(node);
        if (!portGraphic->m_inputWire) {
            if (aboutToBeDeselected || aboutToBeSelected) {
                portGraphic->m_selected = aboutToBeSelected;
            }

            /* If the port is anything other than a boolean port, a change in the signal is represented by starting the
             * color change animation. If it is a boolean signal, just update the pen color */
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

QVariant PortGraphic::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemSelectedChange) {
        updatePen(value.toBool(), !value.toBool());
    }
    return QGraphicsItem::itemChange(change, value);
}

void PortGraphic::updateGeometry() {
    if (m_showValue) {
    }

    QFontMetrics fm(m_font);
    m_textRect = fm.boundingRect(m_widthText);

    if (m_type == PortType::out) {
        m_boundingRect = QRectF(0, 0, m_textRect.width() + PORT_INNER_MARGIN, m_textRect.height() + PORT_INNER_MARGIN);
    } else {
        m_boundingRect = QRectF(GRID_SIZE - m_textRect.width() - PORT_INNER_MARGIN, 0,
                                m_textRect.width() + PORT_INNER_MARGIN, m_textRect.height() + PORT_INNER_MARGIN);
    }

    // Adjust for pen sizes etc.
    m_boundingRect.adjust(-5, -5, 5, 5);
}

const QPen& PortGraphic::getPen() {
    // Only source ports (ports with no input wires) can provide a pen.
    // Sink ports request their pens from their endpoint source port. This call might go through multiple port in/out
    // combinations and component boundaries before reaching the pen.
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
    painter->drawText(QPointF(offset, m_textRect.height() / 2 + PORT_INNER_MARGIN), m_widthText);

    painter->setPen(getPen());
    painter->drawLine(QPointF(0, 0), QPointF(s_portGridWidth * GRID_SIZE, 0));

    painter->restore();

#ifdef VSRTL_DEBUG_DRAW
    DRAW_BOUNDING_RECT(painter)
#endif
}

}  // namespace vsrtl
