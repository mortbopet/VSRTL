#pragma once

#include "vsrtl_graphicsbaseitem.h"
#include "vsrtl_qt_serializers.h"

#include "cereal/cereal.hpp"

#include <QFont>

namespace vsrtl {

class Label : public GraphicsBaseItem<QGraphicsTextItem> {
    Q_OBJECT
public:
    Label(const QString& text, QGraphicsItem* parent, int fontSize = 12);

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;

    void setText(const QString& text);
    void setAlignment(Qt::Alignment alignment);
    void setPointSize(int size);
    void setLocked(bool locked) override;

    template <class Archive>
    void serialize(Archive& archive) {
        prepareGeometryChange();
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
            QString text = toPlainText();
            archive(cereal::make_nvp("Text", text));
            setPlainText(text);
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

        try {
            unsigned alignment = m_alignment;
            archive(cereal::make_nvp("Alignment", alignment));
            m_alignment = static_cast<Qt::Alignment>(alignment);
            setAlignment(m_alignment);
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        applyFormatChanges();
    }

protected:
    void applyFormatChanges();
    void editTriggered();

    QFont m_font;
    Qt::Alignment m_alignment = Qt::AlignCenter;
};

}  // namespace vsrtl
