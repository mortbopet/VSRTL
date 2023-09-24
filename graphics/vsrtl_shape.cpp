#include "vsrtl_shape.h"

namespace vsrtl {
ShapeRegister::ShapeRegister() {
  // Base component
  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Component), {[](QTransform t) {
        QPainterPath shape;
        shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
        return shape;
      }});

  // Signals
  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Wire), {[](QTransform t) {
        QPainterPath shape;
        shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
        return shape;
      }});

  // Register
  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Register),
      {[](QTransform t) {
         QPainterPath shape;
         shape.addPolygon(t.map(QPolygonF({QPointF(0.3, 1), QPointF(0.5, 0.8),
                                           QPointF(0.7, 1), QPointF(0.3, 1)})));
         shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
         shape.setFillRule(Qt::WindingFill);
         return shape;
       },
       QRect(0, 0, 3, 4)});

  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(ClockedComponent),
      {[](QTransform t) {
         QPainterPath shape;
         shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
         shape.setFillRule(Qt::WindingFill);
         return shape;
       },
       QRect(0, 0, 3, 4)});

  // Logic gates
  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(And), {[](QTransform t) {
        constexpr double linearEnd = 0.3;
        QPainterPath shape;
        shape.moveTo(t.map(QPointF(0, 0)));
        shape.lineTo(t.map(QPointF(linearEnd, 0)));
        shape.cubicTo(t.map(QPointF(linearEnd, 0)), t.map(QPointF(1, 0)),
                      t.map(QPointF(1, 0.5)));
        shape.cubicTo(t.map(QPointF(1, 0.5)), t.map(QPointF(1, 1)),
                      t.map(QPointF(linearEnd, 1)));
        shape.lineTo(t.map(QPointF(0, 1)));
        shape.lineTo(t.map(QPointF(0, 0)));
        return shape;
      }});

  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Nand), {[](QTransform t) {
        constexpr double dotRadius = 0.1;
        constexpr double gateRhs = 1.0 - dotRadius * 2;
        constexpr double linearEnd = 0.3;
        QPainterPath shape;
        shape.moveTo(t.map(QPointF(0, 0)));
        shape.lineTo(t.map(QPointF(linearEnd, 0)));
        shape.cubicTo(t.map(QPointF(linearEnd, 0)), t.map(QPointF(gateRhs, 0)),
                      t.map(QPointF(gateRhs, 0.5)));
        shape.cubicTo(t.map(QPointF(gateRhs, 0.5)), t.map(QPointF(gateRhs, 1)),
                      t.map(QPointF(linearEnd, 1)));
        shape.lineTo(t.map(QPointF(0, 1)));
        shape.lineTo(t.map(QPointF(0, 0)));
        QRectF circle =
            t.mapRect(QRectF(QPointF(0, 0), QPointF(dotRadius, dotRadius)));
        shape.addEllipse(t.map(QPointF(gateRhs + dotRadius, 0.5)),
                         circle.width(), circle.height());
        shape.setFillRule(Qt::WindingFill);
        return shape;
      }});

  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Xor), {[](QTransform t) {
        QPainterPath shape;
        shape.moveTo(t.map(QPointF(0, 0)));
        shape.lineTo(t.map(QPointF(0.1, 0)));
        shape.cubicTo(t.map(QPointF(0.1, 0)), t.map(QPointF(1, 0)),
                      t.map(QPointF(1, 0.5)));
        shape.cubicTo(t.map(QPointF(1, 0.5)), t.map(QPointF(1, 1)),
                      t.map(QPointF(0.1, 1)));
        shape.cubicTo(t.map(QPointF(0.1, 1)), t.map(QPointF(0.5, 0.5)),
                      t.map(QPointF(0.1, 0)));
        shape.moveTo(t.map(QPointF(0, 0)));
        shape.cubicTo(t.map(QPointF(0, 0)), t.map(QPointF(0.4, 0.5)),
                      t.map(QPointF(0, 1)));
        shape.cubicTo(t.map(QPointF(0, 1)), t.map(QPointF(0.4, 0.5)),
                      t.map(QPointF(0, 0)));
        shape.setFillRule(Qt::WindingFill);
        return shape;
      }});

  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Or), {[](QTransform t) {
        QPainterPath shape;
        constexpr double linearEnd = 0.3;
        constexpr double cornerIndent = 0.09;
        shape.moveTo(t.map(QPointF(0, 0)));
        shape.lineTo(t.map(QPointF(linearEnd, 0)));
        shape.cubicTo(t.map(QPointF(linearEnd, 0)),
                      t.map(QPointF(1 - cornerIndent, cornerIndent)),
                      t.map(QPointF(1, 0.5)));
        shape.cubicTo(t.map(QPointF(1, 0.5)),
                      t.map(QPointF(1 - cornerIndent, 1 - cornerIndent)),
                      t.map(QPointF(linearEnd, 1)));
        shape.lineTo(t.map(QPointF(0, 1)));
        shape.cubicTo(t.map(QPointF(0, 1)), t.map(QPointF(linearEnd, 0.5)),
                      t.map(QPointF(0, 0)));
        return shape;
      }});

  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Not), {[](QTransform t) {
        QPainterPath shape;
        QRectF circle = t.mapRect(QRectF(QPointF(0, 0), QPointF(0.05, 0.05)));
        shape.addEllipse(t.map(QPointF(0.9, 0.5)), circle.width(),
                         circle.height());
        shape.addPolygon(t.map(QPolygonF(
            {QPointF(0, 0), QPointF(0.8, 0.5), QPointF(0, 1), QPointF(0, 0)})));
        shape.setFillRule(Qt::WindingFill);
        return shape;
      }});

  // Multiplexer
  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Multiplexer), {[](QTransform t) {
        QPainterPath shape;
        shape.addPolygon(
            t.map(QPolygonF({QPointF(0, 0), QPointF(1, 0.2), QPointF(1, 0.8),
                             QPointF(0, 1), QPointF(0, 0)})));
        return shape;
      }});

  // ALU
  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(ALU), {[](QTransform t) {
        QPainterPath shape;
        shape.addPolygon(
            t.map(QPolygonF({QPointF(0, 0), QPointF(1, 0.2), QPointF(1, 0.8),
                             QPointF(0, 1), QPointF(0, 0.65), QPointF(0.2, 0.5),
                             QPointF(0, 0.35), QPointF(0, 0)})));
        return shape;
      }});

  // Adder
  ShapeRegister::registerTypeShape(
      GraphicsTypeFor(Adder), {[](QTransform t) {
        QPainterPath shape;
        shape.addPolygon(
            t.map(QPolygonF({QPointF(0, 0), QPointF(1, 0.2), QPointF(1, 0.8),
                             QPointF(0, 1), QPointF(0, 0.65), QPointF(0.2, 0.5),
                             QPointF(0, 0.35), QPointF(0, 0)})));
        return shape;
      }});
}
} // namespace vsrtl
