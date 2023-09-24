#ifndef VSRTL_COMPONENTGRAPHIC_H
#define VSRTL_COMPONENTGRAPHIC_H

#include <QFont>
#include <QToolButton>

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_gridcomponent.h"
#include "vsrtl_label.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_qt_serializers.h"
#include "vsrtl_shape.h"
#include "vsrtl_wiregraphic.h"

#include "cereal/cereal.hpp"
#include "cereal/types/set.hpp"

#include <qmath.h>

namespace vsrtl {

static inline qreal snapToGrid(qreal v) {
  return round(v / GRID_SIZE) * GRID_SIZE;
}

static inline QRectF gridToScene(QRect gridRect) {
  // Scales a rectangle in grid coordinates to scene coordinates
  QRectF sceneGridRect;
  sceneGridRect.setWidth(gridRect.width() * GRID_SIZE);
  sceneGridRect.setHeight(gridRect.height() * GRID_SIZE);
  return sceneGridRect;
}

static inline QPoint sceneToGrid(QPointF p) {
  return (p / GRID_SIZE).toPoint();
}

static inline QPointF gridToScene(QPoint p) { return p * GRID_SIZE; }

class PortGraphic;
class ComponentButton;

class ComponentGraphic : public GridComponent {
  Q_OBJECT
public:
  ComponentGraphic(SimComponent *c, ComponentGraphic *parent);

  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *item,
             QWidget *) override;

  /**
   * @brief paintOverlay
   * May be implemented by derived classes.
   * Called after ComponentGraphic::paint (painting of the basic shape/outline
   * of the component), wherein derived class specific painting is painted on
   * top
   */
  virtual void paintOverlay(QPainter *, const QStyleOptionGraphicsItem *,
                            QWidget *) {}

  void initialize(bool placeAndRoute = false);
  bool restrictSubcomponentPositioning() const {
    return m_restrictSubcomponentPositioning;
  }
  std::vector<ComponentGraphic *> &getGraphicSubcomponents() {
    return m_subcomponents;
  }
  ComponentGraphic *getParent() const;
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
  void setLocked(bool locked) override;
  bool handlePortGraphicMoveAttempt(const PortGraphic *port,
                                    const QPointF &newBorderPos);

  void setExpanded(bool isExpanded);
  void registerWire(WireGraphic *wire);

  GraphicsBaseItem<QGraphicsItem> *moduleParent() override;

  /**
   * @brief setUserVisible
   * Called whenever the user enables the visibility of a component.
   */
  void setUserVisible(bool state);
  const auto &outputPorts() const { return m_outputPorts; }

private slots:
  /**
   * @brief handleGridPosChange
   * Slot called when position of grid component changed through the grid-layer
   * (ie. through place and route).
   */
  void handleGridPosChange(const QPoint pos);
  void handlePortPosChanged(const vsrtl::SimPort *port);
  void updateGeometry();
  void setIndicatorState(vsrtl::PortGraphic *p, bool enabled);

private:
  void verifySpecialSignals() const;

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
  void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
  QVariant itemChange(QGraphicsItem::GraphicsItemChange change,
                      const QVariant &value) override;
  void paintIndicator(QPainter *painter, PortGraphic *port, QColor color);

  enum class GeometryChange {
    None,
    Resize,
    Expand,
    Collapse,
    ChildJustExpanded,
    ChildJustCollapsed,
  };
  void createSubcomponents(bool doPlaceAndRoute);
  QRectF sceneGridRect() const;

  bool m_restrictSubcomponentPositioning = false;
  bool m_inResizeDragZone = false;
  bool m_resizeDragging = false;
  bool m_isTopLevelSerializedComponent = false;
  /**
   * @brief m_userHidden
   * True if the user has asked to hide this component. Maintains logical
   * hide-state even if the parent component is collaposed, rendering this
   * component as non-visible in the scene.
   */
  bool m_userHidden = false;
  bool userHidden() const { return m_userHidden; }

  std::set<PortGraphic *> m_indicators;
  std::vector<ComponentGraphic *> m_subcomponents;

  /**
   * @brief m_wires
   * WireGraphics always lie within some parent ComponentGraphic. When ports
   * create their wires, they will register their wire with a corresponding
   * ComponentGraphic. These registrations may in turn be used to toggle the
   * visibility of wires inside a ComponentGraphic, based on the expansion state
   * of the ComponentGraphic.
   */
  std::vector<WireGraphic *> m_wires;

  QMap<SimPort *, PortGraphic *> m_inputPorts;
  QMap<SimPort *, PortGraphic *> m_outputPorts;

  Label *m_label;
  std::shared_ptr<QAction> m_labelVisibilityAction;

  // Rectangles
  QPainterPath m_shape;

  QPolygon m_gridPoints;

  QFont m_font;

  QPointF m_expandButtonPos; // Draw position of expand/collapse button in scene
                             // coordinates
  ComponentButton *m_expandButton = nullptr;

public slots:
  void loadLayoutFile(const QString &file);
  void loadLayout();
  void saveLayout();
  void resetWires();
  void parameterDialogTriggered();

public:
  // Bump this when making logic-changing modifications to the serialization
  // logic
  enum LayoutVersions { NoLayoutVersion, v1, LatestLayoutVersion };
  uint32_t m_layoutVersion = 0;

  uint32_t layoutVersion() const override {
    if (m_isTopLevelSerializedComponent) {
      return m_layoutVersion;
    } else {
      return GraphicsBaseItem::layoutVersion();
    }
  }

  template <class Archive>
  void serialize(Archive &archive) {
    setSerializing(true);
    // Serialize the original component name. Wires within the component will
    // reference this when describing parent components, but this component may
    // have different names based on the design which instantiated it. Thus, we
    // need to replace the stored name with the actual name of the component.
    try {
      std::string storedName = getComponent()->getName();
      archive(cereal::make_nvp("Top name", storedName));
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    // Serialize expansion state
    if (hasSubcomponents()) {
      try {
        bool expanded = isExpanded();
        archive(cereal::make_nvp("Expanded", expanded));
        if (expanded != isExpanded()) {
          setExpanded(expanded);
        }
      } catch (const cereal::Exception &e) {
        /// @todo: build an error report
      }
    }

    // Serialize size
    try {
      QRect r = getCurrentComponentRect();
      archive(cereal::make_nvp("Rect", r));
      adjust(r);
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    // Serialize rotation
    try {
      archive(cereal::make_nvp("rot", m_gridRotation));
    } catch (const cereal::Exception &e) {
    }

    // If this is not a top level component, we should serialize its position
    // within its parent component
    if (!m_isTopLevelSerializedComponent) {
      // Serealize position within parent component
      try {
        QPoint p = pos().toPoint();
        archive(cereal::make_nvp("Pos", p));
        setPos(p);
      } catch (const cereal::Exception &e) {
        /// @todo: build an error report
      }

      // Serialize visibility state
      try {
        bool v = isVisible();
        archive(cereal::make_nvp("Visible", v));
        archive(cereal::make_nvp("User hidden", m_userHidden));
        setVisible(v && !m_userHidden);
      } catch (const cereal::Exception &e) {
        /// @todo: build an error report
      }
    }

    /** Serialize port positions
     * @todo this is right now done through GridComponent. In reality, all
     * serialization of grid-component logic (size, positions etc.) should be
     * performed in the grid component.
     */
    serializeBorder(archive);

    // Serialize ports
    for (const auto &pm : {m_inputPorts, m_outputPorts}) {
      for (const auto &p : pm) {
        try {
          archive(cereal::make_nvp(p->getPort()->getName(), *p));
        } catch (const cereal::Exception &e) {
          /// @todo: build an error report
        }
      }
    }

    if (hasSubcomponents()) {
      // Serialize wires from input ports to subcomponents
      // @todo: should this be in port serialization?
      for (auto &p : m_inputPorts) {
        try {
          archive(cereal::make_nvp(p->getPort()->getName() + "_in_wire",
                                   *p->getOutputWire()));
        } catch (const cereal::Exception &e) {
          /// @todo: build an error report
        }
      }

      // Serealize subcomponents
      for (const auto &c : m_subcomponents) {
        try {
          archive(cereal::make_nvp(c->getComponent()->getName(), *c));
        } catch (const cereal::Exception &e) {
          /// @todo: build an error report
        }
      }
    }

    // If this is a top-level component, we should _not_ serialize the output
    // wires. Layouts should be compatible between designs, and the output wire
    // of a top-level component with subcomponents may connect to components
    // which are present in óne design but not another.
    if (!m_isTopLevelSerializedComponent) {
      // Serialize output wire
      for (auto &p : m_outputPorts) {
        try {
          archive(cereal::make_nvp(p->getPort()->getName() + "_out_wire",
                                   *p->getOutputWire()));
        } catch (const cereal::Exception &e) {
          /// @todo: build an error report
        }
      }
    }

    // Serialize component name label
    try {
      archive(cereal::make_nvp("Name label", *m_label));
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    // Serialize indicators
    try {
      std::set<std::string> indicators;
      for (const auto &i : m_indicators) {
        indicators.emplace(i->getPort()->getName());
      }
      archive(cereal::make_nvp("Indicators", indicators));
      for (const auto &pm : {m_inputPorts, m_outputPorts}) {
        for (const auto &ip : pm) {
          if (std::find(indicators.begin(), indicators.end(),
                        ip->getPort()->getName()) != indicators.end()) {
            m_indicators.emplace(ip);
          }
        }
      }
    } catch (const cereal::Exception &e) {
      /// @todo: build an error report
    }

    updateGeometry();
    setSerializing(false);
  }
};
} // namespace vsrtl

#endif // VSRTL_COMPONENTGRAPHIC_H
