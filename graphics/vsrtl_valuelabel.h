#pragma once

#include "vsrtl_displaytype.h"

#include <QGraphicsItem>

namespace vsrtl {

class ValueLabel : public QGraphicsItem {
public:
    ValueLabel(DisplayType& type, unsigned int maxBitWidth, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void setValue(VSRTL_VT_U v);
    void updateText();

private:
    DisplayType& m_type;
    VSRTL_VT_U m_value;
    QString m_text;
    unsigned int m_maxBitWidth;
};

}  // namespace vsrtl
