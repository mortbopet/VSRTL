#ifndef VSRTL_COMPONENTBUTTON_H
#define VSRTL_COMPONENTBUTTON_H

#include <QGraphicsObject>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "vsrtl_graphics_defines.h"

namespace vsrtl {

class ComponentButton : public QGraphicsObject {
  Q_OBJECT
public:
  ComponentButton(QGraphicsItem *parentItem = nullptr)
      : QGraphicsObject(parentItem) {
    setFlags(ItemIsSelectable);
  }
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
    if (boundingRect().contains(event->pos())) {
      m_expanded = !m_expanded;
      emit toggled(m_expanded);
      update();
    }
    QGraphicsItem::mouseReleaseEvent(event);
  }

  QRectF boundingRect() const override {
    return QRectF(0, 0, GRID_SIZE * 2, GRID_SIZE * 2);
  }
  QPainterPath shape() const override {
    QPainterPath p;
    p.addRect(QRectF(GRID_SIZE / 4, GRID_SIZE / 4, GRID_SIZE * 1.75,
                     GRID_SIZE * 1.75));
    return p;
  }

  void setChecked(bool state) {
    m_expanded = state;
    update();
  }

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *,
             QWidget *) override {
    painter->save();
    QPen pen(m_expanded ? BUTTON_COLLAPSE_COLOR : BUTTON_EXPAND_COLOR);
    pen.setWidth(4);
    pen.setCapStyle(Qt::RoundCap);
    painter->setPen(pen);

    const auto rect = boundingRect();

    painter->drawLine(QPointF(GRID_SIZE / 2, rect.height() / 2),
                      QPointF(rect.right() - GRID_SIZE / 2, rect.height() / 2));
    if (!m_expanded) {
      painter->drawLine(
          QPointF(rect.width() / 2, GRID_SIZE / 2),
          QPointF(rect.width() / 2, rect.height() - GRID_SIZE / 2));
    }
    painter->restore();
  }

signals:
  void toggled(bool state);

private:
  bool m_expanded = false;
};

} // namespace vsrtl

#endif // VSRTL_COMPONENTBUTTON_H
