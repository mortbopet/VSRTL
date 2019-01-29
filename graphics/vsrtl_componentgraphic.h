#ifndef VSRTL_COMPONENTGRAPHIC_H
#define VSRTL_COMPONENTGRAPHIC_H

#include <QFont>
#include <QGraphicsItem>
#include <QToolButton>

#include "vsrtl_component.h"

namespace vsrtl {

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

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private slots:
    void setExpandState(bool state);

private:
    enum GeometryChangeFlag { Resize = 1 << 0, Expand = 1 << 1, Collapse = 1 << 2, ChildJustExpanded = 1 << 3 };
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

    bool m_isExpanded = false;
    bool m_hasSubcomponents = false;
    bool m_inDragZone = false;
    bool m_dragging = false;

    std::vector<ComponentGraphic*> m_subcomponents;

    QMap<InputSignalRawPtr, QPointF> m_inputPositionMap;
    QMap<OutputSignalRawPtr, QPointF> m_outputPositionMap;

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
