#include "vsrtl_netlist.h"
#include "ui_vsrtl_netlist.h"
#include "vsrtl_netlistmodel.h"

namespace vsrtl {

Netlist::Netlist(Design& design, QWidget* parent) : QWidget(parent), ui(new Ui::Netlist) {
    ui->setupUi(this);

    m_netlistModel = new NetlistModel(design, this);
    ui->netlistView->setModel(m_netlistModel);
    m_netlistModel->updateNetlistData();

    ui->netlistView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    const QIcon expandIcon = QIcon(":/icons/expand.svg");
    QAction* expandAct = new QAction(expandIcon, "Expand All", this);
    connect(expandAct, &QAction::triggered, ui->netlistView, &QTreeView::expandAll);
    ui->expand->setIcon(expandIcon);
    connect(ui->expand, &QPushButton::clicked, expandAct, &QAction::trigger);

    const QIcon collapseIcon = QIcon(":/icons/collapse.svg");
    QAction* collapseAct = new QAction(collapseIcon, "Collapse All", this);
    connect(collapseAct, &QAction::triggered, ui->netlistView, &QTreeView::collapseAll);
    ui->collapse->setIcon(collapseIcon);
    connect(ui->collapse, &QPushButton::clicked, collapseAct, &QAction::trigger);
}

Netlist::~Netlist() {
    delete ui;
}

void Netlist::reloadNetlist() {
    m_netlistModel->updateNetlistData();
}
}  // namespace vsrtl
