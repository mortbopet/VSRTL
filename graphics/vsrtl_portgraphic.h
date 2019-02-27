#ifndef VSRTL_PORTGRAPHIC_H
#define VSRTL_PORTGRAPHIC_H

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphicsbase.h"

#include <QFont>
#include <QPen>

namespace vsrtl {

class Port;
class WireGraphic;

enum class PortType { in, out };

class PortGraphic : public GraphicsBase {
public:
    PortGraphic(Port* port, PortType type, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void updateGeometry();
    Port* getPort() const { return m_port; }
    void setInputWire(WireGraphic* wire);
    void updateInputWire();
    void updateWireGeometry();

    QPointF getInputPoint() const;
    QPointF getOutputPoint() const;

    const QPen& getPen() const { return m_pen; }

private:
    void updateSlot();
    void initializeSignals();

    bool m_showValue = false;
    ValueDisplayFormat m_valueBase = ValueDisplayFormat::baseTen;

    QRectF m_boundingRect;
    QRectF m_innerRect;

    PortType m_type;
    Port* m_port;

    WireGraphic* m_outputWire = nullptr;
    WireGraphic* m_inputWire = nullptr;

    QString m_widthText;
    QFont m_font;
    QPen m_pen;
};
}  // namespace vsrtl

#endif  // VSRTL_PORTGRAPHIC_H
