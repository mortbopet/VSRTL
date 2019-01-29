#include "vsrtl_widget.h"
#include "ui_vsrtl_widget.h"
#include "vsrtl_architecture.h"

#include <QGraphicsScene>

namespace vsrtl {

VSRTLWidget::VSRTLWidget(QWidget* parent) : QWidget(parent), ui(new Ui::VSRTLWidget) {
    ui->setupUi(this);

    m_view = new VSRTLView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);
    ui->viewLayout->addWidget(m_view);

    m_ch = new CircuitHandler(m_view);
}

VSRTLWidget::~VSRTLWidget() {
    delete ui;
}

void VSRTLWidget::initializeDesign(Architecture* arch) {
    vsrtl::ComponentGraphic* i = new vsrtl::ComponentGraphic(arch);
    addComponent(i);
    i->initialize();

    // Order initial component view
    m_ch->orderSubcomponents(i);
}

void VSRTLWidget::addComponent(ComponentGraphic* g) {
    m_scene->addItem(g);
}
}  // namespace vsrtl
