#pragma once

#include "vsrtl_graphicsbase.h"
#include "vsrtl_port.h"
#include "vsrtl_radix.h"

#include <QGraphicsItem>

namespace vsrtl {

class ValueLabel : public GraphicsBase {
public:
    ValueLabel(Radix& type, const PortBase& port, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void updateText();

private:
    Radix& m_type;
    const PortBase& m_port;
    QString m_text;
    unsigned int m_maxBitWidth;
};

}  // namespace vsrtl
