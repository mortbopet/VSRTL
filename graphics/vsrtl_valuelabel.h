#pragma once

#include "vsrtl_displaytype.h"
#include "vsrtl_port.h"

#include <QGraphicsItem>

namespace vsrtl {

class ValueLabel : public QGraphicsItem {
public:
    ValueLabel(DisplayType& type, const PortBase& port, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void updateText();

private:
    DisplayType& m_type;
    const PortBase& m_port;
    QString m_text;
    unsigned int m_maxBitWidth;
};

}  // namespace vsrtl
