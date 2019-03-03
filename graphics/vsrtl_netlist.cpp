#include "vsrtl_netlist.h"
#include "ui_vsrtl_netlist.h"
#include "vsrtl_netlistmodel.h"

#include <QAction>

namespace vsrtl {

Netlist::Netlist(Design& design, QWidget* parent) : QWidget(parent), ui(new Ui::Netlist) {
    ui->setupUi(this);

    ui->netlistView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->netlistView->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->netlistView->selectionModel(), &QItemSelectionModel::selectionChanged, [] { return 0; });

    m_netlistModel = new NetlistModel(design, this);
    ui->netlistView->setModel(m_netlistModel);
    m_netlistModel->updateNetlistData();

    auto* selectionModel = new QItemSelectionModel(m_netlistModel);
    ui->netlistView->setSelectionModel(selectionModel);
    connect(selectionModel, &QItemSelectionModel::selectionChanged, this, &Netlist::handleViewSelectionChanged);

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

namespace {
void getNetlistDataComponents(const QItemSelection& selected, std::vector<Component*>& c_v) {
    for (const auto& sel : selected.indexes()) {
        TreeItem* item = static_cast<TreeItem*>(sel.internalPointer());
        NetlistData data = item->data(0, Qt::UserRole).value<NetlistData>();
        if (data.component)
            c_v.push_back(data.component);
    }
}
}  // namespace

void Netlist::handleViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    std::vector<Component*> sel_components, desel_components;

    getNetlistDataComponents(selected, sel_components);
    getNetlistDataComponents(deselected, desel_components);

    emit selectionChanged(sel_components, desel_components);
}

Netlist::~Netlist() {
    delete ui;
}

void Netlist::reloadNetlist() {
    m_netlistModel->updateNetlistData();
}
}  // namespace vsrtl
