#ifndef VSRTL_PORTGRAPHIC_H
#define VSRTL_PORTGRAPHIC_H

#include "vsrtl_componentborder.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphicsbaseitem.h"
#include "vsrtl_label.h"
#include "vsrtl_simqobject.h"
#include "vsrtl_valuelabel.h"

#include "../interface/vsrtl_interface.h"

#include "cereal/cereal.hpp"
#include "cereal/types/memory.hpp"

#include <QFont>
#include <QPen>
#include <QPropertyAnimation>

QT_FORWARD_DECLARE_CLASS(QPropertyAnimation)

namespace vsrtl {

class WireGraphic;
class PortPoint;

class PortGraphic : public SimQObject, public GraphicsBaseItem<QGraphicsItem> {
  Q_OBJECT
  Q_PROPERTY(QColor penColor MEMBER m_penColor)
  friend class ValueLabel;
  friend class VSRTLScene;

public:
  PortGraphic(SimPort *port, vsrtl::SimPort::PortType type,
              QGraphicsItem *parent = nullptr);

  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *item,
             QWidget *) override;
  QVariant itemChange(GraphicsItemChange change,
                      const QVariant &value) override;

  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

  void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

  void postSceneConstructionInitialize2() override;
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

  /**
   * @brief setSourceVisible
   * Whenever a component is hidden, all output ports of said component will set
   * the connecting ports as having their source ports non-visible.
   */
  void setSourceVisible(bool visible);

  /**
   * @brief setPortVisible
   * Routine called whenever this port has triggered its visibility, either
   * through scene- or user visibility.
   */
  void setPortVisible(bool visible);

  void updateGeometry();
  SimPort *getPort() const { return m_port; }
  void setInputWire(WireGraphic *wire);
  WireGraphic *getOutputWire() { return m_outputWire; }
  void updateInputWire();
  void updateWireGeometry();
  PortPoint *getPortPoint(vsrtl::SimPort::PortType t) {
    return t == vsrtl::SimPort::PortType::in ? m_inputPortPoint
                                             : m_outputPortPoint;
  }
  QString getTooltipString() const;

  bool userHidden() const { return m_userHidden; }
  void setUserVisible(bool visible);

  QPointF getInputPoint() const;
  QPointF getOutputPoint() const;

  vsrtl::SimPort::PortType getPortType() const { return m_type; }
  void setValueLabelVisible(bool visible);
  void setPortWidthVisible(bool visible);

  const QPen &getPen();

  void setSide(Side side);
  Side getSide() const { return m_side; }

  void modulePositionHasChanged() override;

protected:
  void simUpdateSlot() override;

private slots:
  void updatePenColor();

private:
  void redraw();
  void propagateRedraw();
  void updatePen(bool aboutToBeSelected = false,
                 bool aboutToBeDeselected = false);

  /**
   * @brief m_userHidden
   * True if the user has asked to hide this component. Maintains logical
   * hide-state even if the parent component is collaposed, rendering this
   * component as non-visible in the scene.
   */
  bool m_userHidden = false;

  // m_signalSelected: does not indicate visual selection (ie. isSelected()),
  // but rather whether any port in the port/wire connection of this port has
  // been selected.
  bool m_signalSelected = false;
  bool m_hoverActive = false;

  /**
   * @brief m_sourceVisible (for input ports)
   * true if the outport which this inputport connects to, is visible. If not,
   * the port shall not be drawn. However, the port is still scene-visible to
   * allow for user interaction.
   */
  bool m_sourceVisible = true;
  ValueDisplayFormat m_valueBase = ValueDisplayFormat::baseTen;

  QRectF m_boundingRect;
  QPainterPath m_shape;
  QRectF m_textRect;

  vsrtl::SimPort::PortType m_type;
  SimPort *m_port = nullptr;

  // Used for allowing WireSegments to join up with a port
  PortPoint *m_inputPortPoint = nullptr;
  PortPoint *m_outputPortPoint = nullptr;

  WireGraphic *m_outputWire = nullptr;
  WireGraphic *m_inputWire = nullptr;

  ValueLabel *m_valueLabel = nullptr;

  /**
   * @brief The radix is shared between the port and the value label.
   */
  std::shared_ptr<Radix> m_radix;

  std::unique_ptr<QPropertyAnimation> m_colorAnimation;

  Side m_side = Side::Right;
  Label *m_label = nullptr;
  Label *m_portWidthLabel = nullptr;
  QString m_widthText;
  QFont m_font;
  QPen m_pen;
  QColor m_penColor;
  QPen m_oldPen; // Pen which was previously used for paint(). If a change
                 // between m_oldPen and m_pen is seen, this triggers redrawing
                 // of the connected wires

  std::shared_ptr<QAction> m_showValueAction = nullptr;
  std::shared_ptr<QAction> m_showWidthAction = nullptr;
  std::shared_ptr<QAction> m_showLabelAction = nullptr;
  std::shared_ptr<QAction> m_showAction = nullptr;

public:
  template <class Archive>
  void serialize(Archive &archive) {
    // Serialize labels
    try {
      archive(cereal::make_nvp("Label", *m_label));
      archive(cereal::make_nvp("ValueLabel", *m_valueLabel));
      archive(cereal::make_nvp("UserHidden", m_userHidden));
      setUserVisible(!userHidden());
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    try {
      bool visible = m_showWidthAction->isChecked();
      archive(cereal::make_nvp("PortWidthVisible", visible));
      m_showWidthAction->setChecked(visible);
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    update();
  }
};

} // namespace vsrtl

#endif // VSRTL_PORTGRAPHIC_H
