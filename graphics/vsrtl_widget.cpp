#include "vsrtl_widget.h"
#include "ui_vsrtl_widget.h"
#include "vsrtl_design.h"

#include <memory>

#include <QGraphicsScene>

namespace vsrtl {

VSRTLWidget::VSRTLWidget(Design& arch, QWidget* parent) : m_arch(arch), QWidget(parent), ui(new Ui::VSRTLWidget) {
    ui->setupUi(this);

    m_view = new VSRTLView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);
    ui->viewLayout->addWidget(m_view);

    initializeDesign(arch);
}

VSRTLWidget::~VSRTLWidget() {
    delete ui;
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
