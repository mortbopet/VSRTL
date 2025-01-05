#include "vsrtl_mainwindow.h"
#include "ui_vsrtl_mainwindow.h"
#include "vsrtl_design.h"
#include "vsrtl_netlist.h"
#include "vsrtl_netlistmodel.h"
#include "vsrtl_widget.h"

#include <QAction>
#include <QHeaderView>
#include <QLineEdit>
#include <QSpinBox>
#include <QThread>
#include <QTimer>
#include <QToolBar>

#include <QSplitter>

#include <QTreeView>

namespace vsrtl {

MainWindow::MainWindow(SimDesign &arch, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  setWindowState(Qt::WindowMaximized);
  m_vsrtlWidget = new VSRTLWidget(this);
  m_vsrtlWidget->setDesign(&arch, true);

  m_netlist = new Netlist(arch, this);

  QSplitter *splitter = new QSplitter(this);

  splitter->addWidget(m_netlist);
  splitter->addWidget(m_vsrtlWidget);

  connect(m_netlist, &Netlist::selectionChanged, m_vsrtlWidget,
          &VSRTLWidget::handleSelectionChanged);
  connect(m_vsrtlWidget, &VSRTLWidget::componentSelectionChanged, m_netlist,
          &Netlist::updateSelection);

  setCentralWidget(splitter);

  createToolbar();

  setWindowTitle("VSRTL - Visual Simulation of Register Transfer Logic");
}

MainWindow::~MainWindow() {
  delete ui;
  delete m_vsrtlWidget;
}

void MainWindow::createToolbar() {
  QToolBar *simulatorToolBar = addToolBar("Simulator");

  const QIcon resetIcon = QIcon(":/vsrtl_icons/reset.svg");
  QAction *resetAct = new QAction(resetIcon, "Reset", this);
  connect(resetAct, &QAction::triggered, [this] {
    m_vsrtlWidget->reset();
    m_netlist->reloadNetlist();
  });
  resetAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
  simulatorToolBar->addAction(resetAct);

  const QIcon reverseIcon = QIcon(":/vsrtl_icons/reverse.svg");
  QAction *reverseAct = new QAction(reverseIcon, "Reverse", this);
  connect(reverseAct, &QAction::triggered, [this] {
    m_vsrtlWidget->reverse();
    m_netlist->reloadNetlist();
  });
  reverseAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
  simulatorToolBar->addAction(reverseAct);
  reverseAct->setEnabled(false);
  connect(m_vsrtlWidget, &VSRTLWidget::canReverse, reverseAct,
          &QAction::setEnabled);

  const QIcon clockIcon = QIcon(":/vsrtl_icons/step.svg");
  QAction *clockAct = new QAction(clockIcon, "Clock", this);
  connect(clockAct, &QAction::triggered, [this] {
    m_vsrtlWidget->clock();
    m_netlist->reloadNetlist();
  });
  clockAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
  simulatorToolBar->addAction(clockAct);

  QTimer *timer = new QTimer();
  connect(timer, &QTimer::timeout, clockAct, &QAction::trigger);

  const QIcon startTimerIcon = QIcon(":/vsrtl_icons/step-clock.svg");
  const QIcon stopTimerIcon = QIcon(":/vsrtl_icons/stop-clock.svg");
  QAction *clockTimerAct = new QAction(startTimerIcon, "Auto Clock", this);
  clockTimerAct->setCheckable(true);
  clockTimerAct->setChecked(false);
  connect(clockTimerAct, &QAction::triggered, this, [=] {
    if (timer->isActive()) {
      timer->stop();
      clockTimerAct->setIcon(startTimerIcon);
    } else {
      timer->start();
      clockTimerAct->setIcon(stopTimerIcon);
    }
  });

  simulatorToolBar->addAction(clockTimerAct);

  QSpinBox *stepSpinBox = new QSpinBox();
  stepSpinBox->setRange(1, 10000);
  stepSpinBox->setSuffix(" ms");
  stepSpinBox->setToolTip("Auto clock interval");
  connect(stepSpinBox, qOverload<int>(&QSpinBox::valueChanged),
          [timer](int msec) { timer->setInterval(msec); });
  stepSpinBox->setValue(100);

  simulatorToolBar->addWidget(stepSpinBox);

  QAction *runAct = new QAction(clockIcon, "Run", this);
  runAct->setCheckable(true);
  runAct->setChecked(false);
  connect(runAct, &QAction::triggered, [this](bool state) {
    if (state) {
      auto thread = QThread::create([=] { this->m_vsrtlWidget->run(); });
      thread->start();
    } else {
      this->m_vsrtlWidget->stop();
    }
  });
  simulatorToolBar->addAction(runAct);

  simulatorToolBar->addSeparator();

  const QIcon showNetlistIcon = QIcon(":/vsrtl_icons/list.svg");
  QAction *showNetlist = new QAction(showNetlistIcon, "Show Netlist", this);
  connect(showNetlist, &QAction::triggered, [this] {
    if (m_netlist->isVisible()) {
      m_netlist->hide();
    } else {
      m_netlist->show();
    }
  });
  simulatorToolBar->addAction(showNetlist);

  const QIcon expandAllComponentsIcon = QIcon(":/vsrtl_icons/expandSquare.svg");
  QAction *expandAllComponents =
      new QAction(expandAllComponentsIcon, "Expand all components", this);
  connect(expandAllComponents, &QAction::triggered,
          [this] { m_vsrtlWidget->expandAllComponents(); });
  simulatorToolBar->addAction(expandAllComponents);

} // namespace vsrtl

} // namespace vsrtl
