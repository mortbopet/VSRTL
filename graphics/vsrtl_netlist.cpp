#include "vsrtl_netlist.h"
#include "ui_vsrtl_netlist.h"
#include "vsrtl_netlistdelegate.h"
#include "vsrtl_netlistmodel.h"
#include "vsrtl_registermodel.h"

#include "vsrtl_netlistview.h"

#include <QAction>
#include <QHeaderView>

namespace vsrtl {

Netlist::Netlist(SimDesign &design, QWidget *parent)
    : QWidget(parent), ui(new Ui::Netlist) {
  ui->setupUi(this);

  m_netlistView = new NetlistView<NetlistTreeItem>(this);
  m_netlistModel = new NetlistModel(&design, this);
  m_netlistView->setModel(m_netlistModel);
  ui->netlistViews->addTab(m_netlistView, "Netlist");
  m_netlistModel->invalidate();

  m_registerModel = new RegisterModel(&design, this);
  m_registerView = new NetlistView<RegisterTreeItem>(this);
  m_registerView->setModel(m_registerModel);
  ui->netlistViews->addTab(m_registerView, "Registers");
  m_registerModel->invalidate();

  m_selectionModel = new QItemSelectionModel(m_netlistModel);
  m_netlistView->setSelectionModel(m_selectionModel);
  connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this,
          &Netlist::handleViewSelectionChanged);

  m_netlistView->header()->setSectionResizeMode(NetlistModel::ComponentColumn,
                                                QHeaderView::ResizeToContents);
  m_netlistView->header()->setSectionResizeMode(NetlistModel::IOColumn,
                                                QHeaderView::ResizeToContents);

  m_registerView->header()->setSectionResizeMode(0,
                                                 QHeaderView::ResizeToContents);

  m_netlistView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  m_netlistView->setSelectionBehavior(QAbstractItemView::SelectRows);

  m_registerView->expandAll();
  m_netlistView->expandAll();

  m_registerView->setItemDelegate(new NetlistDelegate(this));

  const QIcon expandIcon = QIcon(":/vsrtl_icons/expand.svg");
  QAction *expandAct = new QAction(expandIcon, "Expand All", this);
  connect(expandAct, &QAction::triggered,
          [=] { this->setCurrentViewExpandState(true); });
  ui->expand->setIcon(expandIcon);
  connect(ui->expand, &QPushButton::clicked, expandAct, &QAction::trigger);

  const QIcon collapseIcon = QIcon(":/vsrtl_icons/collapse.svg");
  QAction *collapseAct = new QAction(collapseIcon, "Collapse All", this);
  connect(collapseAct, &QAction::triggered,
          [=] { this->setCurrentViewExpandState(false); });
  ui->collapse->setIcon(collapseIcon);
  connect(ui->collapse, &QPushButton::clicked, collapseAct, &QAction::trigger);
}

void Netlist::setCurrentViewExpandState(bool state) {
  QTreeView *view = nullptr;
  switch (ui->netlistViews->currentIndex()) {
  case 0: {
    view = m_netlistView;
    break;
  }
  case 1: {
    view = m_registerView;
    break;
  }
  default:
    Q_ASSERT(false);
  }

  if (view) {
    if (state) {
      view->expandAll();
    } else {
      view->collapseAll();
    }
  }
}

void Netlist::updateSelection(const std::vector<SimComponent *> &selected) {
  m_selectionModel->clearSelection();
  for (const auto &c : selected) {
    m_selectionModel->select(m_netlistModel->lookupIndexForComponent(c),
                             QItemSelectionModel::SelectionFlag::Select |
                                 QItemSelectionModel::SelectionFlag::Rows);
  }
}

namespace {
void getIndexComponentPtr(const QItemSelection &selected,
                          std::vector<SimComponent *> &c_v) {
  const auto indexes = selected.indexes();
  for (const auto &sel : std::as_const(indexes)) {
    auto *c =
        static_cast<NetlistTreeItem *>(sel.internalPointer())->m_component;
    if (c) {
      c_v.push_back(c);
    }
  }
}
} // namespace

void Netlist::handleViewSelectionChanged(const QItemSelection &selected,
                                         const QItemSelection &deselected) {
  std::vector<SimComponent *> sel_components, desel_components;

  getIndexComponentPtr(selected, sel_components);
  getIndexComponentPtr(deselected, desel_components);

  emit selectionChanged(sel_components, desel_components);
}

Netlist::~Netlist() { delete ui; }

void Netlist::reloadNetlist() {
  m_netlistModel->invalidate();
  m_registerModel->invalidate();
}
} // namespace vsrtl
