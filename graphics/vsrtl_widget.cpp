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

    m_ch = new CircuitHandler(m_view);

    initializeDesign(arch);
}

VSRTLWidget::~VSRTLWidget() {
    delete ui;
}

void VSRTLWidget::initializeDesign(Design& arch) {
    // Verify the design in case user forgot to
    arch.verifyAndInitialize();

    vsrtl::ComponentGraphic* i = new vsrtl::ComponentGraphic(arch);
    addComponent(i);
    i->initialize();

    // Expand top widget
    i->setExpanded(true);

    // Order initial component view
    m_ch->orderSubcomponents(i);
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
