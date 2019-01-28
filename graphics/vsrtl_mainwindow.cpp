#include "vsrtl_mainwindow.h"
#include "vsrtl_architecture.h"
#include "ui_vsrtl_mainwindow.h"

#include <QGraphicsScene>

namespace vsrtl {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    m_view = new VSRTLView(this);
    m_scene = new QGraphicsScene(this);
    m_view->setScene(m_scene);
    ui->viewLayout->addWidget(m_view);

    m_ch = new CircuitHandler(m_view);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::initializeArchitecture(Architecture* arch) {
    vsrtl::ComponentGraphic* i = new vsrtl::ComponentGraphic(arch);
    addComponent(i);
    i->initialize();

    // Order initial component view
    m_ch->orderSubcomponents(i);
}

void MainWindow::addComponent(ComponentGraphic* g) {
    m_scene->addItem(g);
}
}
