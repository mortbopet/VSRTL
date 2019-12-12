#ifndef VSRTL_COMPONENTGRAPHIC_H
#define VSRTL_COMPONENTGRAPHIC_H

#include <QFont>
#include <QToolButton>

#include "../interface/vsrtl_gfxobjecttypes.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_serializers.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_wiregraphic.h"

#include "cereal/cereal.hpp"

namespace vsrtl {

class PortGraphic;
class Label;
class ComponentButton;

class ComponentGraphic : public GraphicsBase {
public:
    ComponentGraphic(SimComponent* c, QGraphicsItem* parent);

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
    void setShape(const QPainterPath& shape);
    void placeAndRouteSubcomponents();
    bool isExpanded() const { return m_isExpanded; }
    bool restrictSubcomponentPositioning() const { return m_restrictSubcomponentPositioning; }
    SimComponent* getComponent() const { return m_component; }
    std::vector<ComponentGraphic*>& getGraphicSubcomponents() { return m_subcomponents; }
    ComponentGraphic* getParent() const;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void setLocked(bool locked) override;

    void setExpanded(bool isExpanded);
    bool hasSubcomponents() const;

    const auto& outputPorts() const { return m_outputPorts; }

    // Called by vsrtl_core components linked via signal/slot mechanism
    void updateSlot() { update(); }

    template <class Archive>
    void serialize(Archive& archive) {
        // Serialize the original component name. Wires within the component will reference this when describing parent
        // components, but this component may have different names based on the design which instantiated it.
        // Thus, we need to replace the stored name with the actual name of the component.
        try {
            std::string storedName = getComponent()->getName();
            archive(cereal::make_nvp("Top name", storedName));
        } catch (cereal::Exception e) {
            /// @todo: build an error report
        }

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

            // Serialize wires from input ports to subcomponents
            for (auto& p : m_inputPorts) {
                try {
                    archive(cereal::make_nvp(p->getPort()->getName(), *p->getOutputWire()));
                } catch (cereal::Exception e) {
                    /// @todo: build an error report
                }
            }

            // Serealize subcomponents
            for (const auto& c : m_subcomponents) {
                try {
                    archive(cereal::make_nvp(c->getComponent()->getName(), *c));
                } catch (cereal::Exception e) {
                    /// @todo: build an error report
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
                    archive(cereal::make_nvp(p->getPort()->getName(), *p->getOutputWire()));
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

            try {
                bool v = isVisible();
                archive(cereal::make_nvp("Visible", v));
                setVisible(v);
            } catch (cereal::Exception e) {
                /// @todo: build an error report
            }
        }
    }

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
    void updateGeometry(QRect newGridRect, GeometryChange flag);
    QRectF sceneGridRect() const;
    bool rectContainsAllSubcomponents(const QRectF& r) const;
    bool snapToMinGridRect(QRect& r) const;
    QRect subcomponentBoundingGridRect() const;
    QRect adjustedMinGridRect(bool includePorts) const;

    bool m_isExpanded = false;
    bool m_restrictSubcomponentPositioning = false;
    bool m_inResizeDragZone = false;
    bool m_resizeDragging = false;
    bool m_isTopLevelSerializedComponent = false;

    std::vector<ComponentGraphic*> m_subcomponents;
    ComponentGraphic* m_parentComponentGraphic = nullptr;

    QMap<SimPort*, PortGraphic*> m_inputPorts;
    QMap<SimPort*, PortGraphic*> m_outputPorts;

    Label* m_label = nullptr;

    // Rectangles
    const QRect m_minGridRect;  // Minimum component size in grid-coordinates
    QRect m_gridRect;           // Current component size in grid-coordinates
    QPainterPath m_shape;

    QPolygon m_gridPoints;

    QFont m_font;

    QPointF m_expandButtonPos;  // Draw position of expand/collapse button in scene coordinates
    SimComponent* m_component;
    ComponentButton* m_expandButton = nullptr;

protected slots:
    void loadLayout();
    void saveLayout();
    void resetWires();

public:
    /**
     * @brief The Shape struct
     * Component shapes should be scalable in x- and y direction, but may contain complex shapes such as circles.
     * The Shape struct, and shape generation, thus provides an interface for generating (scaling) a QPainterPath,
     * without using QPainte's scale functionality.
     * We avoid using QPainter::scale, given that this also scales the pen, yielding invalid drawings.
     */

    struct Shape {
        std::function<QPainterPath(QTransform)> shapeFunc;
        QRect min_rect;
    };

    static void setComponentShape(std::type_index component, Shape shape) {
        Q_ASSERT(!s_componentShapes.contains(component));
        Q_ASSERT(shape.min_rect.topLeft() == QPoint(0, 0));

        // Ensure that minimum rectangle is snapping to the grid
        s_componentShapes[component] = shape;
    }

    static QPainterPath getComponentShape(std::type_index component, QTransform transform) {
        // If no shape has been registered for the base component type, revert to displaying as a "SimComponent"
        if (!s_componentShapes.contains(component)) {
            return s_componentShapes[GraphicsTypeFor(Component)].shapeFunc(transform);
        }
        return s_componentShapes[component].shapeFunc(transform);
    }

    static QRect getComponentMinGridRect(std::type_index component) {
        // If no shape has been registered for the base component type, revert to displaying as a "SimComponent"
        if (!s_componentShapes.contains(component)) {
            return s_componentShapes[GraphicsTypeFor(Component)].min_rect;
        }
        return s_componentShapes[component].min_rect;
    }

    static QMap<std::type_index, Shape> s_componentShapes;
};
}  // namespace vsrtl

#endif  // VSRTL_COMPONENTGRAPHIC_H
