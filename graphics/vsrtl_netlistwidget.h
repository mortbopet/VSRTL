#ifndef VSRTL_NETLIST_H
#define VSRTL_NETLIST_H

#include <QItemSelection>
#include <QWidget>

#include "../interface/vsrtl_interface.h"
#include "vsrtl_netlistview.h"

namespace vsrtl {

namespace Ui {
class NetlistWidget;
}

class NetlistModel;
class RegisterModel;
class RegisterTreeItem;
class NetlistTreeItem;

class NetlistWidget : public QWidget {
    Q_OBJECT

public:
    explicit NetlistWidget(SimDesign& design, QWidget* parent = 0);
    ~NetlistWidget();

signals:
    void selectionChanged(const std::vector<SimComponent*>& selected, std::vector<SimComponent*>& deselected);

public slots:
    void reloadNetlist();
    void updateSelection(const std::vector<SimComponent*>&);

private slots:
    void handleViewSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    void setCurrentViewExpandState(bool state);

    Ui::NetlistWidget* ui;
    QItemSelectionModel* m_selectionModel;
    NetlistModel* m_netlistModel;
    RegisterModel* m_registerModel;

    NetlistView<RegisterTreeItem>* m_registerView;
    NetlistView<NetlistTreeItem>* m_netlistView;
};
}  // namespace vsrtl

#endif  // VSRTL_NETLIST_H
