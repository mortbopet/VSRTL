#include "vsrtl_mainwindow.h"
#include "ui_vsrtl_mainwindow.h"
#include "vsrtl_architecture.h"
#include "vsrtl_widget.h"

namespace vsrtl {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    m_vsrtlWidget = new VSRTLWidget(this);

    setCentralWidget(m_vsrtlWidget);
}

MainWindow::~MainWindow() {
    delete ui;
    delete m_vsrtlWidget;
}

void MainWindow::loadDesign(Architecture* arch) {
    m_vsrtlWidget->initializeDesign(arch);
}

}  // namespace vsrtl
