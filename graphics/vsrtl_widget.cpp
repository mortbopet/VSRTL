#include "vsrtl_widget.h"
#include "../interface/vsrtl_gfxobjecttypes.h"
#include "ui_vsrtl_widget.h"
#include "vsrtl_portgraphic.h"
#include "vsrtl_scene.h"
#include "vsrtl_shape.h"
#include "vsrtl_view.h"

#include <memory>

#include <QGraphicsScene>

namespace vsrtl {

VSRTLWidget::VSRTLWidget(QWidget* parent) : QWidget(parent), ui(new Ui::VSRTLWidget) {
    ui->setupUi(this);

    // Register shapes for vsrtl-core provided components;
    registerShapes();

    m_view = new VSRTLView(this);
    m_view->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    m_scene = new VSRTLScene(this);
    m_view->setScene(m_scene);
    ui->viewLayout->addWidget(m_view);
    connect(m_scene, &QGraphicsScene::selectionChanged, this, (&VSRTLWidget::handleSceneSelectionChanged));
    m_scene->setBackgroundBrush(QBrush(BACKGROUND_COLOR));
}

void VSRTLWidget::clearDesign() {
    if (m_topLevelComponent) {
        // Clear previous design
        m_topLevelComponent->deleteLater();
    }
    m_design = nullptr;
}

void VSRTLWidget::setDesign(SimDesign* design) {
    if (m_topLevelComponent) {
        // Clear previous design
        m_topLevelComponent->deleteLater();
    }
    m_design = design;
    initializeDesign();
}

void VSRTLWidget::setOutputPortValuesVisible(bool visible) {
    m_scene->setPortValuesVisibleForType(PortType::out, visible);
}

VSRTLWidget::~VSRTLWidget() {
    delete ui;
}

void VSRTLWidget::handleSceneSelectionChanged() {
    std::vector<SimComponent*> selectedComponents;
    std::vector<SimPort*> selectedPorts;
    for (const auto& i : m_scene->selectedItems()) {
        ComponentGraphic* i_c = dynamic_cast<ComponentGraphic*>(i);
        if (i_c) {
            selectedComponents.push_back(&i_c->getComponent());
            continue;
        }
        PortGraphic* i_p = dynamic_cast<PortGraphic*>(i);
        if (i_p) {
            selectedPorts = i_p->getPort()->getPortsInConnection();
            continue;
        }
    }

    emit componentSelectionChanged(selectedComponents);
    emit portSelectionChanged(selectedPorts);
}  // namespace vsrtl

void VSRTLWidget::handleSelectionChanged(const std::vector<SimComponent*>& selected,
                                         std::vector<SimComponent*>& deselected) {
    // Block signals from scene to disable selectionChange emission.
    m_scene->blockSignals(true);
    for (const auto& c : selected) {
        auto* c_g = c->getGraphic<ComponentGraphic>();
        c_g->setSelected(true);
    }
    for (const auto& c : deselected) {
        auto* c_g = c->getGraphic<ComponentGraphic>();
        c_g->setSelected(false);
    }
    m_scene->blockSignals(false);
}

void VSRTLWidget::registerShapes() const {
    // Base component

    ShapeRegister::registerComponentShape(GraphicsIDFor(Component),
                                          {[](QTransform t) {
                                               QPainterPath shape;
                                               shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
                                               return shape;
                                           },
                                           QRect(0, 0, 3, 3)});

    // Signals
    ShapeRegister::registerComponentShape(GraphicsIDFor(Wire),
                                          {[](QTransform t) {
                                               QPainterPath shape;
                                               shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
                                               return shape;
                                           },
                                           QRect(0, 0, 1, 1)});

    // Register
    ShapeRegister::registerComponentShape(
        GraphicsIDFor(Register),
        {[](QTransform t) {
             QPainterPath shape;
             shape.addPolygon(t.map(QPolygonF({QPointF(0.3, 1), QPointF(0.5, 0.8), QPointF(0.7, 1), QPointF(0.3, 1)})));
             shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
             shape.setFillRule(Qt::WindingFill);
             return shape;
         },
         QRect(0, 0, 3, 4)});

    ShapeRegister::registerComponentShape(
        GraphicsIDFor(ClockedComponent),
        {[](QTransform t) {
             QPainterPath shape;
             shape.addPolygon(t.map(QPolygonF({QPointF(0.3, 1), QPointF(0.5, 0.8), QPointF(0.7, 1), QPointF(0.3, 1)})));
             shape.addRect(t.mapRect(QRectF(QPointF(0, 0), QPointF(1, 1))));
             shape.setFillRule(Qt::WindingFill);
             return shape;
         },
         QRect(0, 0, 3, 4)});

    // Logic gates
    ShapeRegister::registerComponentShape(
        GraphicsIDFor(And), {[](QTransform t) {
                                 QPainterPath shape;
                                 shape.cubicTo(QPointF(0, 0), t.map(QPointF(1, 0)), t.map(QPointF(1, 0.5)));
                                 shape.cubicTo(t.map(QPointF(1, 0.5)), t.map(QPointF(1, 1)), t.map(QPointF(0, 1)));
                                 shape.lineTo(QPointF(0, 0));
                                 return shape;
                             },
                             QRect(0, 0, 3, 3)});

    ShapeRegister::registerComponentShape(
        GraphicsIDFor(Xor), {[](QTransform t) {
                                 QPainterPath shape;
                                 shape.moveTo(t.map(QPointF(0.1, 0)));
                                 shape.cubicTo(QPointF(0.1, 0), t.map(QPointF(1, 0)), t.map(QPointF(1, 0.5)));
                                 shape.cubicTo(t.map(QPointF(1, 0.5)), t.map(QPointF(1, 1)), t.map(QPointF(0.1, 1)));
                                 shape.cubicTo(t.map(QPointF(0.1, 1)), t.map(QPointF(0.5, 0.5)),
                                               t.map(QPointF(0.1, 0)));
                                 shape.moveTo(0, 0);
                                 shape.cubicTo(QPointF(0, 0), t.map(QPointF(0.4, 0.5)), t.map(QPointF(0, 1)));
                                 shape.cubicTo(t.map(QPointF(0, 1)), t.map(QPointF(0.4, 0.5)), QPointF(0, 0));
                                 shape.setFillRule(Qt::WindingFill);
                                 return shape;
                             },
                             QRect(0, 0, 3, 3)});

    ShapeRegister::registerComponentShape(
        GraphicsIDFor(Or),
        {[](QTransform t) {
             QPainterPath shape;
             shape.lineTo(t.map(QPointF(0.4, 0)));
             shape.cubicTo(t.map(QPointF(0.4, 0)), t.map(QPointF(0.95, 0.05)), t.map(QPointF(1, 0.5)));
             shape.cubicTo(t.map(QPointF(1, 0.5)), t.map(QPointF(0.95, 0.95)), t.map(QPointF(0.4, 1)));
             shape.lineTo(t.map(QPointF(0, 1)));
             shape.cubicTo(t.map(QPointF(0, 1)), t.map(QPointF(0.4, 0.5)), t.map(QPointF(0, 0)));
             return shape;
         },
         QRect(0, 0, 3, 3)});

    ShapeRegister::registerComponentShape(
        GraphicsIDFor(Not),
        {[](QTransform t) {
             QPainterPath shape;
             QRectF circle = t.mapRect(QRectF(QPointF(0, 0), QPointF(0.05, 0.05)));
             shape.addEllipse(t.map(QPointF(0.9, 0.5)), circle.width(), circle.height());
             shape.addPolygon(t.map(QPolygonF({QPointF(0, 0), QPointF(0.8, 0.5), QPointF(0, 1), QPointF(0, 0)})));
             shape.setFillRule(Qt::WindingFill);
             return shape;
         },
         QRect(0, 0, 3, 3)});

    // Multiplexer
    ShapeRegister::registerComponentShape(
        GraphicsIDFor(Multiplexer),
        {[](QTransform t) {
             QPainterPath shape;
             shape.addPolygon(
                 t.map(QPolygonF({QPointF(0, 0), QPointF(1, 0.2), QPointF(1, 0.8), QPointF(0, 1), QPointF(0, 0)})));
             return shape;
         },
         QRect(0, 0, 2, 5)});

    // ALU
    ShapeRegister::registerComponentShape(
        GraphicsIDFor(ALU), {[](QTransform t) {
                                 QPainterPath shape;
                                 shape.addPolygon(t.map(QPolygonF(
                                     {QPointF(0, 0), QPointF(1, 0.2), QPointF(1, 0.8), QPointF(0, 1), QPointF(0, 0)})));
                                 return shape;
                             },
                             QRect(0, 0, 2, 5)});

    // Adder
    ShapeRegister::registerComponentShape(
        GraphicsIDFor(Adder), {[](QTransform t) {
                                   QPainterPath shape;
                                   shape.addPolygon(t.map(QPolygonF({QPointF(0, 0), QPointF(1, 0.2), QPointF(1, 0.8),
                                                                     QPointF(0, 1), QPointF(0, 0)})));
                                   return shape;
                               },
                               QRect(0, 0, 2, 5)});
}

void VSRTLWidget::initializeDesign() {
    // Verify the design in case user forgot to
    m_design->verifyAndInitialize();

    // Create a ComponentGraphic for the top component. This will expand the component tree and create graphics for all
    // ports, wires etc. within the design. This is done through the initialize call, which must be called after the
    // item has been added to the scene.
    m_topLevelComponent = new ComponentGraphic(*m_design, nullptr);
    addComponent(m_topLevelComponent);
    m_topLevelComponent->initialize();
    // At this point, all graphic items have been created, and the post scene construction initialization may take
    // place. Similar to the initialize call, postSceneConstructionInitialization will recurse through the entire tree
    // which is the graphics items in the scene.
    m_topLevelComponent->postSceneConstructionInitialize1();
    m_topLevelComponent->postSceneConstructionInitialize2();

    // Expand top widget
    m_topLevelComponent->setExpanded(true);
}

void VSRTLWidget::expandAllComponents(ComponentGraphic* fromThis) {
    if (fromThis == nullptr)
        fromThis = m_topLevelComponent;

    // Components are expanded and routed from leaf nodes and up
    for (const auto& sub : fromThis->getGraphicSubcomponents())
        expandAllComponents(sub);

    fromThis->setExpanded(true);
    fromThis->placeAndRouteSubcomponents();
}

bool VSRTLWidget::isRewindable() {
    if (!m_design)
        return false;

    if (m_designCanrewind != m_design->canrewind()) {
        // Rewind state just changed, notify listeners
        m_designCanrewind = m_design->canrewind();
        emit canrewind(m_designCanrewind);
    }
    return m_designCanrewind;
}

void VSRTLWidget::clock() {
    if (m_design) {
        m_design->clock();
        isRewindable();
    }
}

void VSRTLWidget::run() {
    if (m_design) {
        m_design->setEnableSignals(false);
        while (!m_stop) {
            m_design->clock();
        }
        m_stop = false;
        m_design->setEnableSignals(true);
        m_scene->update();
    }
}

void VSRTLWidget::rewind() {
    if (m_design) {
        m_design->rewind();
        isRewindable();
    }
}

void VSRTLWidget::reset() {
    if (m_design) {
        m_design->reset();
        isRewindable();
    }
}

void VSRTLWidget::addComponent(ComponentGraphic* g) {
    m_scene->addItem(g);
}
}  // namespace vsrtl
