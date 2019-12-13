#include "vsrtl_registergraphic.h"
#include "vsrtl_portgraphic.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <QFontMetrics>
namespace vsrtl {

RegisterGraphic::RegisterGraphic(SimComponent* c, QGraphicsItem* parent) : ComponentGraphic(c, parent) {}

void RegisterGraphic::paintOverlay(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* w) {
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    if (lod > 0.4) {
        painter->save();
        QFont f("monospace");
        f.setPointSize(12);
        painter->setFont(f);

        const QString d_text("D");
        const QString q_text("Q");

        const QRectF d_rect(painter->fontMetrics().boundingRect(d_text));
        const QRectF q_rect(painter->fontMetrics().boundingRect(q_text));

        QPointF drawPos;

        // Input port
        for (const auto& ip : m_inputPorts) {
            if (ip->getPort() == m_component->getSpecialPort("in")) {
                drawPos = mapFromItem(ip, ip->getOutputPoint());
                drawPos += QPointF(d_rect.width() / 2.5, -d_rect.height() / 2);
                painter->drawText(drawPos.x(), drawPos.y(), d_rect.width(), d_rect.height(), Qt::AlignCenter, d_text);
            }
        }

        // Output port
        for (const auto& op : m_outputPorts) {
            if (op->getPort() == m_component->getSpecialPort("out")) {
                drawPos = mapFromItem(op, op->getInputPoint());
                drawPos += QPointF(-q_rect.width() - q_rect.width() / 3, -q_rect.height() / 2);
                painter->drawText(drawPos.x(), drawPos.y(), q_rect.width(), q_rect.height(), Qt::AlignCenter, q_text);
            }
        }
        painter->restore();
    }
}
}  // namespace vsrtl
