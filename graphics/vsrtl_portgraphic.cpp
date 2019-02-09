#include "vsrtl_portgraphic.h"
#include "vsrtl_port.h"
#include "vsrtl_wiregraphic.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

namespace {

// static constexpr int c_portHeight = 20;
// static constexpr int c_portWidth = 40;
static constexpr int c_portInnerMargin = 5;
}  // namespace

PortGraphic::PortGraphic(PortBase* port, PortType type, QGraphicsItem* parent) : m_port(port), m_type(type) {
    setParentItem(parent);
    m_widthText = QString::number(port->getWidth() - 1) + ":0";
    m_font = QFont("Monospace", 10);

    updateGeometry();

    initializeSignals();
}

void PortGraphic::initializeSignals() {
    m_outputWire = new WireGraphic(this, m_port->getConnectsFromThis(), this);
}

void PortGraphic::updateInputWire() {
    Q_ASSERT(m_inputWire != nullptr);
    m_inputWire->update();
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

void PortGraphic::updateGeometry() {
    if (m_showValue) {
    }

    QFontMetrics fm(m_font);
    const auto textRect = fm.boundingRect(m_widthText);

    m_boundingRect = QRectF(0, 0, textRect.width() + c_portInnerMargin, textRect.height() + c_portInnerMargin);
}

void PortGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    painter->drawLine(QPointF(0, 0), QPointF(m_boundingRect.width(), 0));
    painter->setFont(m_font);
    const int offset = m_type == PortType::out ? c_portInnerMargin : 0;
    painter->drawText(QPointF(offset, m_boundingRect.height() / 2 + c_portInnerMargin), m_widthText);

    /*
    // Draw bounding rect
    if (lod > 0.2) {
        painter->save();
        painter->setPen(QPen(Qt::black, 1));
        painter->setBrush(QBrush(Qt::Dense3Pattern));
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
