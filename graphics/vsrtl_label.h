#ifndef VSRTL_LABEL_H
#define VSRTL_LABEL_H

#include "vsrtl_graphicsbase.h"

namespace vsrtl {

class Label : public GraphicsBase {
public:
    Label(QString text, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;

private:
    QString m_text;
    QRectF m_textRect;
    const static QFont s_font;
};

}  // namespace vsrtl

#endif  // VSRTL_LABEL_H
