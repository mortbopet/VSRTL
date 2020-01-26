#ifndef VSRTL_LABEL_H
#define VSRTL_LABEL_H

#include "vsrtl_graphicsbase.h"
#include "vsrtl_qt_serializers.h"

#include "cereal/cereal.hpp"

namespace vsrtl {

class Label : public GraphicsBase {
public:
    Label(QString text, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;

    template <class Archive>
    void serialize(Archive& archive) {
        bool v = isVisible();
        archive(cereal::make_nvp("Visible", v));
        setVisible(v);

        archive(cereal::make_nvp("Text", m_text));
        setText(m_text);  // Update text

        QPoint p = pos().toPoint();
        archive(cereal::make_nvp("Pos", p));
        setPos(p);
    }

private:
    QString m_text;
    QRectF m_textRect;
    const static QFont s_font;
};

}  // namespace vsrtl

#endif  // VSRTL_LABEL_H
