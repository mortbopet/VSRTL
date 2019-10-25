#ifndef VSRTL_COMPONENTGRAPHIC_H
#define VSRTL_COMPONENTGRAPHIC_H

#include <QFont>
#include <QToolButton>

#include "vsrtl_component.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_serializers.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_portgraphic.h"

#include "cereal/cereal.hpp"

namespace vsrtl {

class PortGraphic;
class Label;
class ComponentButton;

class ComponentGraphic : public GraphicsBase {
public:
    ComponentGraphic(Component& c);

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
    Component* getComponent() const { return &m_component; }
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
        // Serealize position
        QPoint p = pos().toPoint();
        archive(cereal::make_nvp("Pos", p));
        setPos(p);

        bool expanded = isExpanded();
        archive(cereal::make_nvp("Expanded", expanded));
        if (expanded != isExpanded()) {
            setExpanded(expanded);
        }

        // Serealize ports
        for (auto& p : m_outputPorts) {
            archive(cereal::make_nvp(p->getPort()->getName(), *p));
        }

        // Serealize subcomponents
        for (const auto& c : m_subcomponents) {
            archive(cereal::make_nvp(c->getComponent()->getName(), *c));
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

    std::vector<ComponentGraphic*> m_subcomponents;
    ComponentGraphic* m_parentComponentGraphic = nullptr;

    QMap<PortBase*, PortGraphic*> m_inputPorts;
    QMap<PortBase*, PortGraphic*> m_outputPorts;

    Label* m_label = nullptr;

    // Rectangels:
    const QRect m_minGridRect;  // Minimum component size in grid-coordinates
    QRect m_gridRect;           // Current component size in grid-coordinates
    QPainterPath m_shape;

    QFont m_font;

    QPointF m_expandButtonPos;  // Draw position of expand/collapse button in scene coordinates
    Component& m_component;
    ComponentButton* m_expandButton = nullptr;

protected slots:
    void loadLayout();
    void saveLayout();

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
        // If no shape has been registered for the base component type, revert to displaying as a "Component"
        if (!s_componentShapes.contains(component)) {
            return s_componentShapes[typeid(Component)].shapeFunc(transform);
        }
        return s_componentShapes[component].shapeFunc(transform);
    }

    static QRect getComponentMinGridRect(std::type_index component) {
        // If no shape has been registered for the base component type, revert to displaying as a "Component"
        if (!s_componentShapes.contains(component)) {
            return s_componentShapes[typeid(Component)].min_rect;
        }
        return s_componentShapes[component].min_rect;
    }

    static QMap<std::type_index, Shape> s_componentShapes;
};
}  // namespace vsrtl

#endif  // VSRTL_COMPONENTGRAPHIC_H
