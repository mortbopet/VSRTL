#include "vsrtl_portgraphic.h"
#include "vsrtl_componentgraphic.h"
#include "vsrtl_port.h"
#include "vsrtl_wiregraphic.h"

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

    port->changed.Connect(this, &PortGraphic::updateSlot);

    setFlag(ItemIsSelectable);

    updateGeometry();

    initializeSignals();
}

void PortGraphic::updateSlot() {
    update();
    m_outputWire->update();
}

void PortGraphic::initializeSignals() {
    m_outputWire = new WireGraphic(this, m_port->getConnectsFromThis(), this);
}

void PortGraphic::updateWireGeometry() {
    m_outputWire->prepareGeometryChange();
}

void PortGraphic::updateInputWire() {
    Q_ASSERT(m_inputWire != nullptr);
    // Signal parent port of input wire to update - this will update all outgoing wires from the port
    m_inputWire->getFromPort()->updateWireGeometry();
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

QVariant PortGraphic::itemChange(GraphicsItemChange change, const QVariant& value) {
    /* Port selection
     * When selecting any port, all ports connected together with this port should also be selected. This triggers port
     * selection in the netlist, as well as redrawing of all ports as being currently selected
     * m_selecting acts as a lock for preventing infinite recursion when a port tries to select its connected ports
     * before its own itemChange call has returned.
     */
    if (m_selecting)
        return QVariant();

    m_selecting = true;
    if (change == QGraphicsItem::ItemSelectedChange) {
        // Notify joined ports to also be selected
        if (m_inputWire) {
            m_inputWire->getFromPort()->setSelected(value.toBool());
        }
        if (m_outputWire) {
            for (const auto& p : m_outputWire->getToPorts()) {
                p->setSelected(value.toBool());
            }
        }
    }
    m_selecting = false;

    return QGraphicsItem::itemChange(change, value);
}

void PortGraphic::updateGeometry() {
    if (m_showValue) {
    }

    QFontMetrics fm(m_font);
    const auto textRect = fm.boundingRect(m_widthText);

    m_boundingRect = QRectF(0, 0, textRect.width() + c_portInnerMargin, textRect.height() + c_portInnerMargin);
}

void PortGraphic::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (isSelected()) {
        setSelected(false);
    } else {
        QGraphicsItem::mousePressEvent(event);
    }
}
void PortGraphic::mouseReleaseEvent(QGraphicsSceneMouseEvent*) {
    // We override QGraphicsItem::mouseReleaseEvent(event) to do nothing, to avoid trigerring deselection upon mouse
    // button release, which does not mix with the mechanism which selects all connected ports on port mouse press.
}

void PortGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    bool redrawOutputWire = false;

    painter->save();
    painter->setFont(m_font);
    const int offset = m_type == PortType::out ? c_portInnerMargin : 0;
    painter->drawText(QPointF(offset, m_boundingRect.height() / 2 + c_portInnerMargin), m_widthText);

    QPen newPen = m_pen;
    newPen.setStyle(Qt::SolidLine);
    // Update pen based on port state
    if (m_inputWire && m_inputWire->getFromPort()->isVisible()) {
        newPen = m_inputWire->getPen();
    } else {
        QColor c("#636363");
        if (option->state & QStyle::State_Selected) {
            c = QColor("#fef160");
        } else if (m_port->getWidth() == 1) {
            c = static_cast<bool>(*m_port) ? QColor("#6EEB83") : c;
        } else {
            c = s_defaultWireColor;
        }
        newPen.setColor(c);
    }
    if (newPen.color() != m_pen.color())
        redrawOutputWire = true;

    m_pen = newPen;
    painter->setPen(m_pen);

    painter->drawLine(QPointF(0, 0), QPointF(m_boundingRect.width(), 0));
    painter->restore();

    if (redrawOutputWire) {
        m_outputWire->update();
    }

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
