#include "vsrtl_mainwindow.h"
#include "ui_vsrtl_mainwindow.h"
#include "vsrtl_architecture.h"
#include "vsrtl_widget.h"

#include <QAction>
#include <QLineEdit>
#include <QToolBar>

namespace vsrtl {

MainWindow::MainWindow(Architecture& arch, QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    m_vsrtlWidget = new VSRTLWidget(arch, this);

    setCentralWidget(m_vsrtlWidget);

    createToolbar();
}

MainWindow::~MainWindow() {
    delete ui;
    delete m_vsrtlWidget;
}

void MainWindow::createToolbar() {
    QToolBar* simulatorToolBar = addToolBar("Simulator");

    QAction* resetAct = new QAction("Reset", this);
    connect(resetAct, &QAction::triggered, m_vsrtlWidget, &VSRTLWidget::reset);
    simulatorToolBar->addAction(resetAct);

    QAction* clockAct = new QAction("Clock", this);
    connect(clockAct, &QAction::triggered, m_vsrtlWidget, &VSRTLWidget::clock);
    simulatorToolBar->addAction(clockAct);

    QLineEdit* cycleCount = new QLineEdit();
    cycleCount->setReadOnly(true);
    simulatorToolBar->addWidget(cycleCount);
}

}  // namespace vsrtl
