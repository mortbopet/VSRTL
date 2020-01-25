#ifndef VSRTL_PORTGRAPHIC_H
#define VSRTL_PORTGRAPHIC_H

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_valuelabel.h"

#include "../interface/vsrtl_interface.h"

#include "cereal/cereal.hpp"

#include <QFont>
#include <QPen>

QT_FORWARD_DECLARE_CLASS(QPropertyAnimation)

namespace vsrtl {

class WireGraphic;
class PortPoint;

enum class PortType { in, out };

class PortGraphic : public GraphicsBase {
    Q_OBJECT
    Q_PROPERTY(QColor penColor MEMBER m_penColor)

public:
    PortGraphic(SimPort* port, PortType type, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void postSceneConstructionInitialize2() override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void updateGeometry();
    SimPort* getPort() const { return m_port; }
    void setInputWire(WireGraphic* wire);
    WireGraphic* getOutputWire() { return m_outputWire; }
    void updateInputWire();
    void updateWireGeometry();
    PortPoint* getPointGraphic() { return m_portPoint; }
    QString getTooltipString() const;

    void setOutwireVisible(bool state);

    QPointF getInputPoint() const;
    QPointF getOutputPoint() const;

    PortType getPortType() const { return m_type; }
    void setLabelVisible(bool visible);

    const QPen& getPen();

    static int portGridWidth() { return s_portGridWidth; }
    int gridIndex() const { return m_gridIndex; }
    void setGridIndex(int i) { m_gridIndex = i; }

private slots:
    void updatePenColor();

private:
    void redraw();
    void propagateRedraw();
    void updatePen(bool aboutToBeSelected = false, bool aboutToBeDeselected = false);
    void updateSlot();

    // m_signalSelected: does not indicate visual selection (ie. isSelected()), but rather whether any port in the
    // port/wire connection of this port has been selected.
    bool m_signalSelected = false;
    bool m_hoverActive = false;
    ValueDisplayFormat m_valueBase = ValueDisplayFormat::baseTen;

    static int s_portGridWidth;
    int m_gridIndex;

    QRectF m_boundingRect;
    QRectF m_textRect;

    PortType m_type;
    SimPort* m_port;

    // Used for allowing WireSegments to join up with a port
    PortPoint* m_portPoint = nullptr;

    WireGraphic* m_outputWire = nullptr;
    WireGraphic* m_inputWire = nullptr;

    ValueLabel* m_valueLabel = nullptr;

    Radix m_Radix = Radix::Hex;

    QPropertyAnimation* m_colorAnimation;

    QString m_widthText;
    QFont m_font;
    QPen m_pen;
    QColor m_penColor;
    QPen m_oldPen;  // Pen which was previously used for paint(). If a change between m_oldPen and m_pen is seen, this
                    // triggers redrawing of the connected wires
};

}  // namespace vsrtl

#endif  // VSRTL_PORTGRAPHIC_H
