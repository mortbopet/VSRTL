#ifndef VSRTL_COMPONENTGRAPHIC_H
#define VSRTL_COMPONENTGRAPHIC_H

#include <QFont>
#include <QToolButton>

#include "vsrtl_component.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbase.h"
#include "vsrtl_placeroute.h"

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

    bool isExpanded() const { return m_isExpanded; }

    Component* getComponent() const { return &m_component; }

    void setExpanded(bool isExpanded);
    bool hasSubcomponents() const;
    void setGridPos(const QPoint& p);
    QRect adjustedMinGridRect(bool includePorts, bool moveToParentGridPos) const;

    const auto& outputPorts() const { return m_outputPorts; }

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
    void placeAndRouteSubcomponents();
    QRect subcomponentBoundingGridRect() const;

    ComponentGraphic* getParent() const;

    // Called by vsrtl_core component linked via signal/slot mechanism
    void updateSlot() { update(); }

    bool m_isExpanded = false;
    bool m_inResizeDragZone = false;
    bool m_resizeDragging = false;

    std::vector<ComponentGraphic*> m_subcomponents;

    QMap<Port*, PortGraphic*> m_inputPorts;
    QMap<Port*, PortGraphic*> m_outputPorts;

    Label* m_label = nullptr;

    // Rectangels:
    const QRect m_minGridRect;  // Minimum component size in grid-coordinates
    QPoint m_gridPos;           // Component positioning in its parent grid coordinate system
    QRect m_gridRect;           // Current component size in grid-coordinates
    QPainterPath m_shape;

    QFont m_font;

    QPointF m_expandButtonPos;  // Draw position of expand/collapse button in scene coordinates

    Component& m_component;

    ComponentButton* m_expandButton = nullptr;
    pr::RoutingRegions m_routingRegions;
    pr::Netlist m_netlist;

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
