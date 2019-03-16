#include "vsrtl_netlist.h"
#include "ui_vsrtl_netlist.h"
#include "vsrtl_netlistdelegate.h"
#include "vsrtl_netlistmodel.h"
#include "vsrtl_registermodel.h"

#include <QAction>

namespace vsrtl {

Netlist::Netlist(Design& design, QWidget* parent) : QWidget(parent), ui(new Ui::Netlist) {
    ui->setupUi(this);

    ui->netlistView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->netlistView->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->registerView->setSelectionBehavior(QAbstractItemView::SelectItems);

    m_netlistModel = new NetlistModel(design, this);
    ui->netlistView->setModel(m_netlistModel);
    m_netlistModel->updateNetlistData();

    m_registerModel = new RegisterModel(design, this);
    ui->registerView->setModel(m_registerModel);
    m_registerModel->updateNetlistData();

    m_selectionModel = new QItemSelectionModel(m_netlistModel);
    ui->netlistView->setSelectionModel(m_selectionModel);
    connect(m_selectionModel, &QItemSelectionModel::selectionChanged, this, &Netlist::handleViewSelectionChanged);

    ui->netlistView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->registerView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    ui->registerView->expandAll();
    ui->netlistView->expandAll();

    ui->registerView->setItemDelegate(new NetlistDelegate(this));

    const QIcon expandIcon = QIcon(":/icons/expand.svg");
    QAction* expandAct = new QAction(expandIcon, "Expand All", this);
    connect(expandAct, &QAction::triggered, [=] { this->setCurrentViewExpandState(true); });
    ui->expand->setIcon(expandIcon);
    connect(ui->expand, &QPushButton::clicked, expandAct, &QAction::trigger);

    const QIcon collapseIcon = QIcon(":/icons/collapse.svg");
    QAction* collapseAct = new QAction(collapseIcon, "Collapse All", this);
    connect(collapseAct, &QAction::triggered, [=] { this->setCurrentViewExpandState(false); });
    ui->collapse->setIcon(collapseIcon);
    connect(ui->collapse, &QPushButton::clicked, collapseAct, &QAction::trigger);
}

void Netlist::setCurrentViewExpandState(bool state) {
    QTreeView* view;
    switch (ui->netlistViews->currentIndex()) {
        case 0: {
            view = ui->netlistView;
            break;
        }
        case 1: {
            view = ui->registerView;
            break;
        }
        default:
            Q_ASSERT(false);
    }

    if (state) {
        view->expandAll();
    } else {
        view->collapseAll();
    }
}

void Netlist::updateSelection(const std::vector<Component*>& selected) {
    m_selectionModel->clearSelection();
    for (const auto& c : selected) {
        m_selectionModel->select(m_netlistModel->lookupIndexForComponent(c),
                                 QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
    }
}

namespace {
void getIndexComponentPtr(const QItemSelection& selected, std::vector<Component*>& c_v) {
    for (const auto& sel : selected.indexes()) {
        NetlistItem* item = static_cast<NetlistItem*>(sel.internalPointer());
        auto c = item->data(0, NetlistRoles::CorePtr).value<Component*>();
        if (c)
            c_v.push_back(c);
    }
}
}  // namespace

void Netlist::handleViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
    std::vector<Component*> sel_components, desel_components;

    getIndexComponentPtr(selected, sel_components);
    getIndexComponentPtr(deselected, desel_components);

    emit selectionChanged(sel_components, desel_components);
}

Netlist::~Netlist() {
    delete ui;
}

void Netlist::reloadNetlist() {
    m_netlistModel->updateNetlistData();
    m_registerModel->updateNetlistData();
}
}  // namespace vsrtl
