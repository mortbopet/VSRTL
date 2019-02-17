#ifndef VSRTL_COMPONENTGRAPHIC_H
#define VSRTL_COMPONENTGRAPHIC_H

#include <QFont>
#include <QToolButton>

#include <map>

#include "vsrtl_component.h"
#include "vsrtl_graphics_defines.h"
#include "vsrtl_graphics_util.h"
#include "vsrtl_graphicsbase.h"

namespace vsrtl {

class PortGraphic;
class Label;

class ComponentGraphic : public GraphicsBase {
public:
    ComponentGraphic(Component& c);

    QRectF boundingRect() const override;
    QRectF baseRect() const { return m_baseRect; }
    QRectF sceneBaseRect() const;
    /** @todo remember to implement shape()
     * We do not want our bounding rectangle to be the shape of the object, since the IO pins of an object shouldnt be
     * factored into collisions - wherease shape() should be ONLY the object
     */
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

    const Component& getComponent() const { return m_component; }
    void setExpanded(bool isExpanded);
    bool hasSubcomponents() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    enum GeometryChangeFlag {
        Resize = 1 << 0,
        Expand = 1 << 1,
        Collapse = 1 << 2,
        ChildJustExpanded = 1 << 3,
        ChildJustCollapsed = 1 << 4
    };
    void updateBaseRect(GeometryChangeFlag flag);
    void updateTextPosition();
    void createSubcomponents();
    void setIOPortPositions();
    void setLabelPosition();
    void updateGeometry(GeometryChangeFlag flag);
    void updateSubcomponentRect();
    void updateBoundingRect();
    void getSubGraphicsItems(QGraphicsItemGroup& g);
    bool rectContainsAllSubcomponents(const QRectF& r) const;
    bool snapToSubcomponentRect(QRectF& r) const;
    void orderSubcomponents();
    void initializePorts();
    void updateDrawShape();
    ComponentGraphic* getParent() const;

    // Called by vsrtl_core component linked via signal/slot mechanism
    void updateSlot() { update(); }

    bool m_isExpanded = false;
    bool m_inResizeDragZone = false;
    bool m_resizeDragging = false;

    std::map<ComponentGraphic*, Component*> m_subcomponents;

    QMap<PortBase*, PortGraphic*> m_inputPorts;
    QMap<PortBase*, PortGraphic*> m_outputPorts;

    Label* m_label;

    QRectF m_savedBaseRect = QRectF();
    const QRectF m_minRect;
    QRectF m_baseRect;
    QRectF m_boundingRect;
    QPainterPath m_shape;
    QRectF m_textRect;
    QRectF m_subcomponentRect;
    QFont m_font;
    QString m_displayText;

    QPointF m_textPos;
    QPointF m_expandButtonPos;

    Component& m_component;

    QToolButton* m_expandButton;
    QGraphicsProxyWidget* m_expandButtonProxy;

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
        QRectF min_rect;
    };

    static void setComponentShape(std::type_index component, Shape shape) {
        Q_ASSERT(!s_componentShapes.contains(component));
        Q_ASSERT(shape.min_rect.topLeft() == QPointF(0, 0));

        // Ensure that minimum rectangle is snapping to the grid
        roundUp(shape.min_rect, GRID_SIZE);
        s_componentShapes[component] = shape;
    }

    static QPainterPath getComponentShape(std::type_index component, QTransform transform) {
        // If no shape has been registered for the base component type, revert to displaying as a "Component"
        if (!s_componentShapes.contains(component)) {
            return s_componentShapes[typeid(Component)].shapeFunc(transform);
        }
        return s_componentShapes[component].shapeFunc(transform);
    }

    static QRectF getComponentMinRect(std::type_index component) {
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
