#include "vsrtl_mainwindow.h"
#include "ui_vsrtl_mainwindow.h"
#include "vsrtl_design.h"
#include "vsrtl_netlistmodel.h"
#include "vsrtl_widget.h"

#include <QAction>
#include <QHeaderView>
#include <QLineEdit>
#include <QToolBar>

#include <QSplitter>

#include <QTreeView>

namespace vsrtl {

MainWindow::MainWindow(Design& arch, QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    m_vsrtlWidget = new VSRTLWidget(arch, this);

    m_netlistView = new QTreeView(this);

    m_netlistModel = new NetlistModel(arch, this);
    m_netlistView->setModel(m_netlistModel);
    m_netlistModel->updateNetlistData();

    m_netlistView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

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

    const QIcon resetIcon = QIcon(":/icons/reset.svg");
    QAction* resetAct = new QAction(resetIcon, "Reset", this);
    connect(resetAct, &QAction::triggered, [this] {
        m_vsrtlWidget->reset();
        m_netlistModel->updateNetlistData();
    });
    resetAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_R));
    simulatorToolBar->addAction(resetAct);

    const QIcon clockIcon = QIcon(":/icons/step.svg");
    QAction* clockAct = new QAction(clockIcon, "Clock", this);
    connect(clockAct, &QAction::triggered, [this] {
        m_vsrtlWidget->clock();
        m_netlistModel->updateNetlistData();
    });
    clockAct->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_C));
    simulatorToolBar->addAction(clockAct);

    QLineEdit* cycleCount = new QLineEdit();
    cycleCount->setReadOnly(true);
    simulatorToolBar->addWidget(cycleCount);

    simulatorToolBar->addSeparator();

    const QIcon showNetlistIcon = QIcon(":/icons/list.svg");
    QAction* showNetlist = new QAction(showNetlistIcon, "Show Netlist", this);
    connect(showNetlist, &QAction::triggered, [this] {
        if (m_netlistView->isVisible()) {
            m_netlistView->hide();
        } else {
            m_netlistView->show();
        }
    });
    simulatorToolBar->addAction(showNetlist);

    const QIcon expandIcon = QIcon(":/icons/expand.svg");
    QAction* expandAct = new QAction(expandIcon, "Expand All", this);
    connect(expandAct, &QAction::triggered, m_netlistView, &QTreeView::expandAll);
    simulatorToolBar->addAction(expandAct);

    const QIcon collapseIcon = QIcon(":/icons/collapse.svg");
    QAction* collapseAll = new QAction(collapseIcon, "Collapse All", this);
    connect(collapseAll, &QAction::triggered, m_netlistView, &QTreeView::collapseAll);
    simulatorToolBar->addAction(collapseAll);
}

}  // namespace vsrtl
