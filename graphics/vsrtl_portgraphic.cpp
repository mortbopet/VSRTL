#include "vsrtl_portgraphic.h"
#include "vsrtl_port.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

namespace {

static constexpr int c_portHeight = 20;
static constexpr int c_portWidth = 40;
static constexpr int c_portInnerMargin = 5;

}  // namespace

PortGraphic::PortGraphic(PortBase* port, PortType type, QGraphicsItem* parent) : m_port(port), m_type(type) {
    setParentItem(parent);

    updateGeometry();
}

QRectF PortGraphic::boundingRect() const {
    return m_boundingRect;
}

void PortGraphic::updateGeometry() {
    if (m_showValue) {
    }

    m_boundingRect = QRectF(0, 0, c_portWidth, c_portHeight);
    m_innerRect = m_boundingRect;
    m_innerRect.setTopRight(m_innerRect.topRight() + QPointF(-c_portInnerMargin, c_portInnerMargin));
    m_innerRect.setBottomLeft(m_innerRect.bottomLeft() - QPointF(-c_portInnerMargin, c_portInnerMargin));
}

void PortGraphic::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

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
}

}  // namespace vsrtl
