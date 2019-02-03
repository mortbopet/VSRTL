#ifndef VSRTL_PORTGRAPHIC_H
#define VSRTL_PORTGRAPHIC_H

#include <QGraphicsItem>
#include "vsrtl_graphics_defines.h"

namespace vsrtl {

class PortBase;

enum class PortType { in, out };

class PortGraphic : public QGraphicsItem {
public:
    PortGraphic(PortBase* port, PortType type, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void updateGeometry();

private:
    bool m_showValue = false;
    ValueDisplayFormat m_valueBase = ValueDisplayFormat::baseTen;

    QRectF m_boundingRect;
    QRectF m_innerRect;

    PortType m_type;
    PortBase* m_port;
};
}  // namespace vsrtl

#endif  // VSRTL_PORTGRAPHIC_H
