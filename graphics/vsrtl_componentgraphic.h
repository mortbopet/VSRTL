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

static inline QPointF gridToScene(QPoint p) {
    return p * GRID_SIZE;
}

class PortGraphic;
class ComponentButton;

class ComponentGraphic : public GridComponent {
public:
    ComponentGraphic(SimComponent& c, ComponentGraphic* parent);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;

    /**
     * @brief paintOverlay
     * May be implemented by derived classes.
     * Called after ComponentGraphic::paint (painting of the basic shape/outline of the component), wherein derived
     * class specific painting is painted on top
     */
    virtual void paintOverlay(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) {}

    void initialize();
    bool restrictSubcomponentPositioning() const { return m_restrictSubcomponentPositioning; }
    std::vector<ComponentGraphic*>& getGraphicSubcomponents() { return m_subcomponents; }
    ComponentGraphic* getParent() const;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void setLocked(bool locked) override;
    bool handlePortGraphicMoveAttempt(const PortGraphic* port, const QPointF& newBorderPos);

    void setExpanded(bool isExpanded);

    /**
     * @brief setUserVisible
     * Called whenever the user enables the visibility of a component.
     */
    void setUserVisible(bool state);
    const auto& outputPorts() const { return m_outputPorts; }

    // Called by vsrtl_core components linked via signal/slot mechanism
    void updateSlot() { update(); }

private slots:
    /**
     * @brief handleGridPosChange
     * Slot called when position of grid component changed through the grid-layer (ie. through place and route).
     */
    void handleGridPosChange(const QPoint pos);
    void handlePortPosChanged(const SimPort* port);
    void updateGeometry();
    void setIndicatorState(PortGraphic* p, bool enabled);

private:
    void verifySpecialSignals() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    enum class GeometryChange {
        None,
        Resize,
        Expand,
        Collapse,
        ChildJustExpanded,
        ChildJustCollapsed,
    };
    void createSubcomponents();
    QRectF sceneGridRect() const;

    bool m_restrictSubcomponentPositioning = false;
    bool m_inResizeDragZone = false;
    bool m_resizeDragging = false;
    bool m_isTopLevelSerializedComponent = false;
    /**
     * @brief m_userHidden
     * True if the user has asked to hide this component. Maintains logical hide-state even
     * if the parent component is collaposed, rendering this component as non-visible in the scene.
     */
    bool m_userHidden = false;
    bool userHidden() const { return m_userHidden; }

    std::set<PortGraphic*> m_indicators;
    std::vector<ComponentGraphic*> m_subcomponents;
    ComponentGraphic* m_parentComponentGraphic = nullptr;

    QMap<SimPort*, PortGraphic*> m_inputPorts;
    QMap<SimPort*, PortGraphic*> m_outputPorts;

    Label* m_label = nullptr;

    // Rectangles
    QPainterPath m_shape;

    QPolygon m_gridPoints;

    QFont m_font;

    QPointF m_expandButtonPos;  // Draw position of expand/collapse button in scene coordinates
    ComponentButton* m_expandButton = nullptr;

public slots:
    void loadLayoutFile(const QString& file);
    void loadLayout();
    void saveLayout();
    void resetWires();

public:
    template <class Archive>
    void serialize(Archive& archive) {
        // Serialize the original component name. Wires within the component will reference this when describing parent
        // components, but this component may have different names based on the design which instantiated it.
        // Thus, we need to replace the stored name with the actual name of the component.
        try {
            std::string storedName = getComponent().getName();
            archive(cereal::make_nvp("Top name", storedName));
        } catch (cereal::Exception e) {
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
            } catch (cereal::Exception e) {
                /// @todo: build an error report
            }
        }

        // Serialize size
        try {
            QRect r = getCurrentComponentRect();
            archive(cereal::make_nvp("Rect", r));
            adjust(r);
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        // Serialize ports
        for (const auto& pm : {m_inputPorts, m_outputPorts}) {
            for (const auto& p : pm) {
                try {
                    archive(cereal::make_nvp(p->getPort()->getName(), *p));
                } catch (cereal::Exception e) {
                    /// @todo: build an error report
                }
            }
        }

        if (hasSubcomponents()) {
            // Serialize wires from input ports to subcomponents
            // @todo: should this be in port serialization?
            for (auto& p : m_inputPorts) {
                try {
                    archive(cereal::make_nvp(p->getPort()->getName() + "_in_wire", *p->getOutputWire()));
                } catch (cereal::Exception e) {
                    /// @todo: build an error report
                }
            }

            // Serealize subcomponents
            for (const auto& c : m_subcomponents) {
                try {
                    archive(cereal::make_nvp(c->getComponent().getName(), *c));
                } catch (cereal::Exception e) {
                    Q_ASSERT(false);
                }
            }
        }

        // If this is a top-level component, we should _not_ serialize the output wires. Layouts should be compatible
        // between designs, and the output wire of a top-level component with subcomponents may connect to components
        // which are present in óne design but not another.
        if (!m_isTopLevelSerializedComponent) {
            // Serialize output wire
            for (auto& p : m_outputPorts) {
                try {
                    archive(cereal::make_nvp(p->getPort()->getName() + "_out_wire", *p->getOutputWire()));
                } catch (cereal::Exception e) {
                    /// @todo: build an error report
                }
            }
        }

        // If this is not a top level component, we should serialize its position within its parent component
        if (!m_isTopLevelSerializedComponent) {
            // Serealize position within parent component
            try {
                QPoint p = pos().toPoint();
                archive(cereal::make_nvp("Pos", p));
                setPos(p);
            } catch (cereal::Exception e) {
                /// @todo: build an error report
            }

            // Serialize visibility state
            try {
                bool v = isVisible();
                archive(cereal::make_nvp("Visible", v));
                setVisible(v);
                m_userHidden = !v;
            } catch (cereal::Exception e) {
                /// @todo: build an error report
            }
        }

        /** Serialize port positions
         * @todo this is right now done through GridComponent. In reality, all serialization of grid-component logic
         * (size, positions etc.) should be performed in the grid component.
         */
        serializeBorder(archive);

        // Serialize component name label
        try {
            archive(cereal::make_nvp("Name label", *m_label));
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

        // Serialize indicators
        try {
            std::set<std::string> indicators;
            for (const auto& i : m_indicators) {
                indicators.emplace(i->getPort()->getName());
            }
            archive(cereal::make_nvp("Indicators", indicators));
            for (const auto& ip : m_inputPorts) {
                if (std::find(indicators.begin(), indicators.end(), ip->getPort()->getName()) != indicators.end()) {
                    m_indicators.emplace(ip);
                }
            }
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }
    }
};
}  // namespace vsrtl

#endif  // VSRTL_COMPONENTGRAPHIC_H
