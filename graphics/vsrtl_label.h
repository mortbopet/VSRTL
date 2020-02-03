#pragma once

#include "vsrtl_graphicsbase.h"
#include "vsrtl_qt_serializers.h"

#include "cereal/cereal.hpp"

#include <QFont>

namespace vsrtl {

class Label : public GraphicsBase {
public:
    Label(const QString& text, QGraphicsItem* parent, int fontSize = 12);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

    void setText(const QString& text);
    void setColor(const QColor& color);

    template <class Archive>
    void serialize(Archive& archive) {
        try {
            bool v = isVisible();
            archive(cereal::make_nvp("Visible", v));
            setVisible(v);
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        try {
            bool bold = m_font.bold();
            archive(cereal::make_nvp("Bold", bold));
            m_font.setBold(bold);
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        try {
            bool italic = m_font.italic();
            archive(cereal::make_nvp("Italic", italic));
            m_font.setItalic(italic);
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        try {
            int ptSize = m_font.pointSize();
            archive(cereal::make_nvp("PtSize", ptSize));
            m_font.setPointSize(ptSize);
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        try {
            archive(cereal::make_nvp("Text", m_text));
            setText(m_text);  // Update text
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        try {
            QPointF p = pos();
            archive(cereal::make_nvp("Pos", p));
            setPos(p);
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }
    }

private:
    void editTriggered();

    QString m_text;
    QRectF m_textRect;
    QFont m_font;
    QColor m_color;
};

}  // namespace vsrtl
