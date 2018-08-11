#ifndef RIPES_COMPONENTGRAPHIC_H
#define RIPES_COMPONENTGRAPHIC_H

#include <QFont>
#include <QGraphicsItem>
#include <QToolButton>

#include "ripes_component.h"

namespace ripes {

class ComponentGraphic : public QGraphicsItem {
public:
    ComponentGraphic(Component* c);

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

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private slots:
    void setExpandState(bool state);

private:
    void calculateBaseRect();
    void calculateTextPosition();
    void createSubcomponents();
    void calculateIOPositions();
    void calculateGeometry();
    void calculateSubcomponentRect();
    void getSubGraphicsItems(QGraphicsItemGroup& g);

    bool m_isExpanded = false;
    bool m_hasSubcomponents = false;

    std::vector<ComponentGraphic*> m_subcomponents;

    QMap<SignalBase***, QPointF> m_inputPositionMap;
    QMap<SignalBase*, QPointF> m_outputPositionMap;

    QRectF m_baseRect;
    QRectF m_boundingRect;
    QRectF m_textRect;
    QRectF m_subcomponentRect;
    QFont m_font;
    QString m_displayText;

    QPointF m_textPos;
    QPointF m_expandButtonPos;

    Component* m_component;

    QToolButton* m_expandButton;
    QGraphicsProxyWidget* m_expandButtonProxy;
};
}  // namespace ripes

#endif  // RIPES_COMPONENTGRAPHIC_H
