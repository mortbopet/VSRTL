#ifndef VSRTL_COMPONENTGRAPHIC_H
#define VSRTL_COMPONENTGRAPHIC_H

#include <QFont>
#include <QGraphicsItem>
#include <QToolButton>

#include "vsrtl_component.h"

namespace vsrtl {

class PortGraphic;

class ComponentGraphic : public QGraphicsItem {
public:
    ComponentGraphic(Component& c);

    QRectF boundingRect() const override;
    QRectF baseRect() const { return m_baseRect; }
    QRectF sceneBaseRect() const;
    /** @todo remember to implement shape()
     * We do not want our bounding rectangle to be the shape of the object, since the IO pins of an object shouldnt be
     * factored into collisions - wherease shape() should be ONLY the object
     */
    // QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* item, QWidget*) override;
    void initialize();

    const Component& getComponent() const { return m_component; }
    void setExpanded(bool isExpanded);
    bool hasSubcomponents() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    enum GeometryChangeFlag {
        Resize = 1 << 0,
        Expand = 1 << 1,
        Collapse = 1 << 2,
        ChildJustExpanded = 1 << 3,
        ChildJustCollapsed = 1 << 4
    };
    void calculateBaseRect(GeometryChangeFlag flag);
    void calculateTextPosition();
    void createSubcomponents();
    void calculateIOPositions();
    void calculateGeometry(GeometryChangeFlag flag);
    void calculateSubcomponentRect();
    void calculateBoundingRect();
    void getSubGraphicsItems(QGraphicsItemGroup& g);
    bool rectContainsAllSubcomponents(const QRectF& r) const;
    bool snapToSubcomponentRect(QRectF& r) const;
    void orderSubcomponents();
    ComponentGraphic* getParent() const;

    bool m_isExpanded = false;
    bool m_inResizeDragZone = false;
    bool m_resizeDragging = false;

    std::map<ComponentGraphic*, Component*> m_subcomponents;

    QMap<PortBase*, PortGraphic*> m_inputPorts;
    QMap<PortBase*, PortGraphic*> m_outputPorts;

    QRectF m_savedBaseRect = QRectF();
    QRectF m_baseRect;
    QRectF m_boundingRect;
    QRectF m_textRect;
    QRectF m_subcomponentRect;
    QFont m_font;
    QString m_displayText;

    QPointF m_textPos;
    QPointF m_expandButtonPos;

    Component& m_component;

    QToolButton* m_expandButton;
    QGraphicsProxyWidget* m_expandButtonProxy;
};
}  // namespace vsrtl

#endif  // VSRTL_COMPONENTGRAPHIC_H
