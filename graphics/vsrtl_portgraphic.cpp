#include "vsrtl_portgraphic.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_port.h"
#include "vsrtl_traversal_util.h"
#include "vsrtl_wiregraphic.h"

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

namespace {

// static constexpr int c_portHeight = 20;
// static constexpr int c_portWidth = 40;
static constexpr int c_portInnerMargin = 5;
static QColor s_defaultWireColor("#636363");
}  // namespace

PortGraphic::PortGraphic(Port* port, PortType type, QGraphicsItem* parent) : m_port(port), m_type(type) {
    port->registerGraphic(this);
    setParentItem(parent);
    m_widthText = QString::number(port->getWidth() - 1) + ":0";
    m_font = QFont("Monospace", 8);
    m_pen.setWidth(WIRE_WIDTH);
    m_pen.setColor(s_defaultWireColor);
    m_pen.setCapStyle(Qt::RoundCap);

    port->changed.Connect(this, &PortGraphic::updateSlot);

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

void PortGraphic::propagateRedraw() {
    m_port->traverseToSinks([=](Port* port) {
        auto* portGraphic = getGraphic<PortGraphic*>(port);
        portGraphic->redraw();
    });
}

/**
 * @brief PortGraphic::postSceneConstructionInitialize2
 * With all wires having registered themselves with their connected ports, the inital state of the pen for a source port
 * may now be set
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
    return QPointF(m_boundingRect.left(), 0);
}

QPointF PortGraphic::getOutputPoint() const {
    return QPointF(m_boundingRect.right(), 0);
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

            // This is a source port. Update pen based on current state
            QColor c("#636363");
            // Selection check is based on whether item is currently selected or about to be selected (via itemChange())
            if (portGraphic->m_selected) {
                c = QColor("#fef160");
            } else if (m_port->getWidth() == 1) {
                c = static_cast<bool>(*m_port) ? QColor("#6EEB83") : c;
            } else {
                c = s_defaultWireColor;
            }
            portGraphic->m_pen.setColor(c);

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
    const auto textRect = fm.boundingRect(m_widthText);

    m_boundingRect = QRectF(0, 0, textRect.width() + c_portInnerMargin, textRect.height() + c_portInnerMargin);
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
    const int offset = m_type == PortType::out ? c_portInnerMargin : 0;
    painter->drawText(QPointF(offset, m_boundingRect.height() / 2 + c_portInnerMargin), m_widthText);

    painter->setPen(getPen());
    painter->drawLine(QPointF(0, 0), QPointF(m_boundingRect.width(), 0));

    painter->restore();

    /*
    // Draw bounding rect
    if (isSelected()) {
        painter->save();
        painter->setPen(QPen(Qt::red, 1));
        painter->drawRect(m_boundingRect);
        painter->restore();
    }

    // Draw interior.
    if (lod >= 0.5) {
        painter->save();
        painter->setBrush(QBrush(Qt::white));
        painter->drawRect(m_innerRect);
        painter->restore();
        if (m_showValue) {
            if (m_port->getWidth() == 1) {
                // Boolean value - indicate this with colors instead of a number
            } else {
                // Draw according to the current base
            }
        } else {
            // Draw an arrow inside, indicating direction
            painter->save();
            QPainterPath path;
            path.moveTo(c_portInnerMargin, c_portInnerMargin + c_portHeight / 4);
            path.lineTo(c_portInnerMargin + c_portWidth / 4, c_portHeight / 4);
            path.lineTo(c_portInnerMargin + c_portWidth / 4, c_portInnerMargin + c_portWidth / 4);
            path.lineTo(c_portInnerMargin + c_portWidth / 4, c_portHeight / 4);
            path.lineTo(c_portInnerMargin + c_portWidth / 2, 0);
            path.lineTo(c_portInnerMargin + c_portWidth / 4, c_portHeight / 4);
            QPen pen = painter->pen();
            pen.setJoinStyle(Qt::RoundJoin);
            painter->setPen(pen);
            painter->drawPath(path);
            painter->restore();
        }
    }
    */
}

}  // namespace vsrtl
