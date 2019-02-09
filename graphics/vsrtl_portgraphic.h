#ifndef VSRTL_PORTGRAPHIC_H
#define VSRTL_PORTGRAPHIC_H

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphicsbase.h"

#include <QFont>

namespace vsrtl {

class PortBase;
class WireGraphic;

enum class PortType { in, out };

class PortGraphic : public GraphicsBase {
public:
    PortGraphic(PortBase* port, PortType type, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void updateGeometry();
    PortBase* getPort() const { return m_port; }
    void setInputWire(WireGraphic* wire);
    void updateInputWire();

    QPointF getInputPoint() const;
    QPointF getOutputPoint() const;

private:
    void initializeSignals();

    bool m_showValue = false;
    ValueDisplayFormat m_valueBase = ValueDisplayFormat::baseTen;

    QRectF m_boundingRect;
    QRectF m_innerRect;

    PortType m_type;
    PortBase* m_port;

    WireGraphic* m_outputWire = nullptr;
    WireGraphic* m_inputWire = nullptr;

    QString m_widthText;
    QFont m_font;
};
}  // namespace vsrtl

#endif  // VSRTL_PORTGRAPHIC_H
