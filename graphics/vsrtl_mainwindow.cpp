#include "vsrtl_mainwindow.h"
#include "ui_vsrtl_mainwindow.h"
#include "vsrtl_architecture.h"
#include "vsrtl_netlistmodel.h"
#include "vsrtl_widget.h"

#include <QAction>
#include <QHeaderView>
#include <QLineEdit>
#include <QToolBar>

#include <QSplitter>

#include <QTreeView>

namespace vsrtl {

MainWindow::MainWindow(Architecture& arch, QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    m_vsrtlWidget = new VSRTLWidget(arch, this);

    m_netlistView = new QTreeView(this);

    m_netlistModel = new NetlistModel(arch, this);
    m_netlistView->setModel(m_netlistModel);

    QSplitter* splitter = new QSplitter(this);

    splitter->addWidget(m_vsrtlWidget);
    splitter->addWidget(m_netlistView);

    setCentralWidget(splitter);

    createToolbar();

    setWindowTitle("VSRTL - Visual Simulation of Register Transfer Logic");
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

    QAction* showNetlist = new QAction("Show Netlist", this);
    connect(showNetlist, &QAction::triggered, [this] {
        if (m_netlistView->isVisible()) {
            m_netlistView->hide();
        } else {
            m_netlistView->show();
        }
    });
    simulatorToolBar->addAction(showNetlist);
}

}  // namespace vsrtl
