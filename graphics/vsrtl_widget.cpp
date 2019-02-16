#include "vsrtl_widget.h"
#include "ui_vsrtl_widget.h"
#include "vsrtl_design.h"

#include "vsrtl_shape.h"

#include "vsrtl_core.h"

#include <memory>

#include <QGraphicsScene>

namespace vsrtl {

VSRTLWidget::VSRTLWidget(Design& arch, QWidget* parent) : m_arch(arch), QWidget(parent), ui(new Ui::VSRTLWidget) {
    ui->setupUi(this);

    // Register shapes for vsrtl-core provided components;
    registerShapes();

    m_view = new VSRTLView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);
    ui->viewLayout->addWidget(m_view);

    initializeDesign(arch);
}

VSRTLWidget::~VSRTLWidget() {
    delete ui;
}

void VSRTLWidget::registerShapes() const {
    // Base component

    ComponentGraphic::setComponentShape("Component",
                                        {[](QTransform t) {
                                             QPainterPath shape;
                                             shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
                                             return shape;
                                         },
                                         QRectF(0, 0, 25, 25)});

    // Register
    ComponentGraphic::setComponentShape(
        "Register",
        {[](QTransform t) {
             QPainterPath shape;
             shape.addPolygon(t.map(QPolygonF({QPointF(0.3, 1), QPointF(0.5, 0.8), QPointF(0.7, 1), QPointF(0.3, 1)})));
             shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
             shape.setFillRule(Qt::WindingFill);
             return shape;
         },
         QRectF(0, 0, 30, 45)});

    // Constant
    ComponentGraphic::setComponentShape("Constant", {[](QTransform t) {
                                                         QPainterPath shape;
                                                         shape.addRoundRect(
                                                             t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))), 35);
                                                         return shape;
                                                     },
                                                     QRectF(0, 0, 15, 15)});

    // Logic gates
    ComponentGraphic::setComponentShape(
        "And", {[](QTransform t) {
                    QPainterPath shape;
                    shape.cubicTo(QPointF(0, 0), t.map(QPointF(1, 0)), t.map(QPointF(1, 0.5)));
                    shape.cubicTo(t.map(QPointF(1, 0.5)), t.map(QPointF(1, 1)), t.map(QPointF(0, 1)));
                    shape.lineTo(QPointF(0, 0));
                    return shape;
                },
                QRectF(0, 0, 40, 20)});

    // Logic gates
    ComponentGraphic::setComponentShape(
        "Xor", {[](QTransform t) {
                    QPainterPath shape;
                    shape.moveTo(t.map(QPointF(0.1, 0)));
                    shape.cubicTo(QPointF(0.1, 0), t.map(QPointF(1, 0)), t.map(QPointF(1, 0.5)));
                    shape.cubicTo(t.map(QPointF(1, 0.5)), t.map(QPointF(1, 1)), t.map(QPointF(0.1, 1)));
                    shape.cubicTo(t.map(QPointF(0.1, 1)), t.map(QPointF(0.5, 0.5)), t.map(QPointF(0.1, 0)));
                    shape.moveTo(0, 0);
                    shape.cubicTo(QPointF(0, 0), t.map(QPointF(0.4, 0.5)), t.map(QPointF(0, 1)));
                    shape.cubicTo(t.map(QPointF(0, 1)), t.map(QPointF(0.4, 0.5)), QPointF(0, 0));
                    shape.setFillRule(Qt::WindingFill);
                    return shape;
                },
                QRectF(0, 0, 40, 20)});

    ComponentGraphic::setComponentShape(
        "Or", {[](QTransform t) {
                   QPainterPath shape;
                   shape.lineTo(t.map(QPointF(0.4, 0)));
                   shape.cubicTo(t.map(QPointF(0.4, 0)), t.map(QPointF(0.95, 0.05)), t.map(QPointF(1, 0.5)));
                   shape.cubicTo(t.map(QPointF(1, 0.5)), t.map(QPointF(0.95, 0.95)), t.map(QPointF(0.4, 1)));
                   shape.lineTo(t.map(QPointF(0, 1)));
                   shape.cubicTo(t.map(QPointF(0, 1)), t.map(QPointF(0.4, 0.5)), t.map(QPointF(0, 0)));
                   return shape;
               },
               QRectF(0, 0, 40, 20)});

    // Multiplexer
    ComponentGraphic::setComponentShape(
        "Multiplexer", {[](QTransform t) {
                            QPainterPath shape;
                            shape.addPolygon(t.map(QPolygonF(
                                {QPointF(0, 0), QPointF(1, 0.2), QPointF(1, 0.8), QPointF(0, 1), QPointF(0, 0)})));
                            return shape;
                        },
                        QRectF(0, 0, 20, 20)});

    // ALU
    ComponentGraphic::setComponentShape(
        "ALU", {[](QTransform t) {
                    QPainterPath shape;
                    shape.addPolygon(t.map(
                        QPolygonF({QPointF(0, 0), QPointF(1, 0.2), QPointF(1, 0.8), QPointF(0, 1), QPointF(0, 0)})));
                    return shape;
                },
                QRectF(0, 0, 30, 20)});
}

void VSRTLWidget::initializeDesign(Design& arch) {
    // Verify the design in case user forgot to
    arch.verifyAndInitialize();

    // Create a ComponentGraphic for the top component. This will expand the component tree and create graphics for all
    // ports, wires etc. within the design. This is done through the initialize call, which must be called after the
    // item has been added to the scene.
    vsrtl::ComponentGraphic* i = new vsrtl::ComponentGraphic(arch);
    addComponent(i);
    i->initialize();
    // At this point, all graphic items have been created, and the post scene construction initialization may take
    // place. Similar to the initialize call, postSceneConstructionInitialization will recurse through the entire tree
    // which is the graphics items in the scene.
    i->postSceneConstructionInitialize();

    // Expand top widget
    i->setExpanded(true);
}

void VSRTLWidget::clock() {
    m_arch.clock();
}

void VSRTLWidget::reset() {
    m_arch.reset();
}

void VSRTLWidget::addComponent(ComponentGraphic* g) {
    m_scene->addItem(g);
}
}  // namespace vsrtl
