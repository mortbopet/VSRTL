#ifndef VSRTL_COMPONENTGRAPHIC_H
#define VSRTL_COMPONENTGRAPHIC_H

#include <QFont>
#include <QToolButton>

#include "core/vsrtl_component.h"
#include "eda/gridcomponent.h"
#include "eda/vsrtl_placeroute.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbase.h"

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
    QPainterPath m_shape;
    QFont m_font;
    QPointF m_expandButtonPos;  // Draw position of expand/collapse button in scene coordinates

    Component& m_component;

    eda::GridComponent m_gridComponent;

    ComponentButton* m_expandButton = nullptr;
    eda::RoutingRegions m_routingRegions;
    eda::Netlist m_netlist;

public:
};
}  // namespace vsrtl

#endif  // VSRTL_COMPONENTGRAPHIC_H
