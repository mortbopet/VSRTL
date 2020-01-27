#pragma once

#include "vsrtl_graphicsbase.h"
#include "vsrtl_radix.h"

#include "../interface/vsrtl_interface.h"

#include <QFont>
#include <QGraphicsItem>

namespace vsrtl {

class ValueLabel : public GraphicsBase {
public:
    ValueLabel(Radix& type, const SimPort* port, QGraphicsItem* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void updateText();

private:
    Radix& m_type;
    const SimPort* m_port;
    QString m_text;
    unsigned int m_maxBitWidth;
    static QFont s_font;
    static QFont s_constantFont;
};

}  // namespace vsrtl
