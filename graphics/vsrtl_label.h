#pragma once

#include "vsrtl_graphicsbase.h"
#include "vsrtl_qt_serializers.h"

#include "cereal/cereal.hpp"

#include <QFont>

namespace vsrtl {

class Label : public GraphicsBase {
public:
    Label(QString text, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void setText(const QString& text);

    template <class Archive>
    void serialize(Archive& archive) {
        bool v = isVisible();
        archive(cereal::make_nvp("Visible", v));
        setVisible(v);

        bool bold = m_font.bold();
        archive(cereal::make_nvp("Bold", bold));
        m_font.setBold(bold);

        bool italic = m_font.italic();
        archive(cereal::make_nvp("Italic", italic));
        m_font.setItalic(italic);

        int ptSize = m_font.pointSize();
        archive(cereal::make_nvp("PtSize", ptSize));
        m_font.setPointSize(ptSize);

        archive(cereal::make_nvp("Text", m_text));
        setText(m_text);  // Update text

        QPointF p = pos();
        archive(cereal::make_nvp("Pos", p));
        setPos(p);
    }

private:
    QString m_text;
    QRectF m_textRect;
    QFont m_font;
};

}  // namespace vsrtl
