#include "vsrtl_label.h"

#include <QFontMetrics>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace vsrtl {

const QFont Label::s_font = QFont("Monospace", 12);

Label::Label(QString text, QGraphicsItem* parent) : m_text(text) {
    setParentItem(parent);
    QFontMetricsF fm(s_font);
    m_textRect = fm.boundingRect(m_text);
}

QRectF Label::boundingRect() const {
    return m_textRect.translated(-m_textRect.width() / 2, -m_textRect.height() / 2);
}

void Label::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*) {
    const qreal lod = option->levelOfDetailFromTransform(painter->worldTransform());

    // Draw text
    if (lod >= 0.35) {
        QPointF offset(-m_textRect.width() / 2, -m_textRect.height() / 2);
        painter->setFont(s_font);
        painter->save();
        painter->drawText(offset, m_text);
        painter->restore();
    }
}

}  // namespace vsrtl
