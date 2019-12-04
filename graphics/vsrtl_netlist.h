#ifndef VSRTL_NETLIST_H
#define VSRTL_NETLIST_H

#include <QItemSelection>
#include <QWidget>

#include "vsrtl_design.h"
#include "vsrtl_netlistview.h"

namespace vsrtl {
using namespace core;

namespace Ui {
class Netlist;
}

class NetlistModel;
class RegisterModel;
class RegisterTreeItem;
class NetlistTreeItem;

class Netlist : public QWidget {
    Q_OBJECT

public:
    explicit Netlist(Design& design, QWidget* parent = 0);
    ~Netlist();

signals:
    void selectionChanged(const std::vector<Component*>& selected, std::vector<Component*>& deselected);

public slots:
    void reloadNetlist();
    void updateSelection(const std::vector<Component*>&);

private slots:
    void handleViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    void setCurrentViewExpandState(bool state);

    Ui::Netlist* ui;
    QItemSelectionModel* m_selectionModel;
    NetlistModel* m_netlistModel;
    RegisterModel* m_registerModel;

    NetlistView<RegisterTreeItem>* m_registerView;
    NetlistView<NetlistTreeItem>* m_netlistView;
};
}  // namespace vsrtl

#endif  // VSRTL_NETLIST_H
