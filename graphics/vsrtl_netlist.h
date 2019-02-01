#ifndef VSRTL_NETLIST_H
#define VSRTL_NETLIST_H

#include <QWidget>

#include "vsrtl_design.h"

namespace vsrtl {

namespace Ui {
class Netlist;
}

class NetlistModel;

class Netlist : public QWidget {
    Q_OBJECT

public:
    explicit Netlist(Design& design, QWidget* parent = 0);
    ~Netlist();

public slots:
    void reloadNetlist();

private:
    Ui::Netlist* ui;
    NetlistModel* m_netlistModel;
};
}  // namespace vsrtl

#endif  // VSRTL_NETLIST_H
