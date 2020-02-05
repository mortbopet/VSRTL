#ifndef VSRTL_PORTGRAPHIC_H
#define VSRTL_PORTGRAPHIC_H

#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_label.h"
#include "vsrtl_valuelabel.h"

#include "../interface/vsrtl_interface.h"

#include "cereal/cereal.hpp"

#include <QFont>
#include <QPen>
#include <QPropertyAnimation>

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
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void postSceneConstructionInitialize2() override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    /**
     * @brief setSourceVisible
     * Whenever a component is hidden, all output ports of said component will set the connecting ports as having their
     * source ports non-visible.
     */
    void setSourceVisible(bool visible);

    /**
     * @brief setPortVisible
     * Routine called whenever this port has triggered its visibility, either through scene- or user visibility.
     */
    void setPortVisible(bool visible);

    void updateGeometry();
    SimPort* getPort() const { return m_port; }
    void setInputWire(WireGraphic* wire);
    WireGraphic* getOutputWire() { return m_outputWire; }
    void updateInputWire();
    void updateWireGeometry();
    PortPoint* getPointGraphic() { return m_portPoint; }
    QString getTooltipString() const;

    void setOutwireVisible(bool state);
    bool userHidden() const { return m_userHidden; }
    void setUserVisible(bool visible);

    QPointF getInputPoint() const;
    QPointF getOutputPoint() const;

    PortType getPortType() const { return m_type; }
    void setValueLabelVisible(bool visible);
    void setPortWidthVisible(bool visible);

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

    /**
     * @brief m_userHidden
     * True if the user has asked to hide this component. Maintains logical hide-state even
     * if the parent component is collaposed, rendering this component as non-visible in the scene.
     */
    bool m_userHidden = false;

    // m_signalSelected: does not indicate visual selection (ie. isSelected()), but rather whether any port in the
    // port/wire connection of this port has been selected.
    bool m_signalSelected = false;
    bool m_hoverActive = false;

    /**
     * @brief m_sourceVisible (for input ports)
     * true if the outport which this inputport connects to, is visible. If not, the port shall not be drawn. However,
     * the port is still scene-visible to allow for user interaction.
     */
    bool m_sourceVisible = true;
    ValueDisplayFormat m_valueBase = ValueDisplayFormat::baseTen;

    static int s_portGridWidth;
    int m_gridIndex;

    QRectF m_boundingRect;
    QRectF m_textRect;

    PortType m_type;
    SimPort* m_port = nullptr;
    bool m_portWidthVisible = false;

    // Used for allowing WireSegments to join up with a port
    PortPoint* m_portPoint = nullptr;

    WireGraphic* m_outputWire = nullptr;
    WireGraphic* m_inputWire = nullptr;

    ValueLabel* m_valueLabel = nullptr;

    Radix m_radix = Radix::Hex;

    std::unique_ptr<QPropertyAnimation> m_colorAnimation;

    Label* m_label = nullptr;
    QString m_widthText;
    QFont m_font;
    QPen m_pen;
    QColor m_penColor;
    QPen m_oldPen;  // Pen which was previously used for paint(). If a change between m_oldPen and m_pen is seen, this
                    // triggers redrawing of the connected wires

public:
    template <class Archive>
    void serialize(Archive& archive) {
        // Serialize port name label
        try {
            archive(cereal::make_nvp("Label", *m_label));
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        // Serialize port value label visibility
        try {
            bool valueLabelVisible = m_valueLabel->isVisible();
            archive(cereal::make_nvp("ValueLabelVisible", valueLabelVisible));
            setValueLabelVisible(valueLabelVisible);
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        // Serialize port width label visibility
        try {
            archive(cereal::make_nvp("PortWidthVisible", m_portWidthVisible));
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        // Serialize port visibility state
        try {
            archive(cereal::make_nvp("UserHidden", m_userHidden));
            setUserVisible(!userHidden());
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        update();
    }
};

}  // namespace vsrtl

#endif  // VSRTL_PORTGRAPHIC_H
