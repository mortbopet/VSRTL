#ifndef VSRTL_NETLISTMODEL_H
#define VSRTL_NETLISTMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "vsrtl_netlistmodelbase.h"
#include "vsrtl_treeitem.h"

#include "../interface/vsrtl_interface.h"

namespace vsrtl {

class NetlistModel : public NetlistModelBase<NetlistTreeItem> {
  Q_OBJECT

public:
  enum columns {
    ComponentColumn,
    IOColumn,
    ValueColumn,
    WidthColumn,
    NUM_COLUMNS
  };
  NetlistModel(SimDesign *arch, QObject *parent = nullptr);

  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;

  QModelIndex lookupIndexForComponent(SimComponent *c) const;

public slots:
  void invalidate() override;

private:
  void addPortToComponent(SimPort *port, NetlistTreeItem *parent,
                          PortDirection);
  void loadDesign(NetlistTreeItem *parent, SimDesign *design);
  void loadDesignRecursive(NetlistTreeItem *parent, SimComponent *component);
  SimComponent *getParentComponent(const QModelIndex &index) const;
  std::map<SimComponent *, NetlistTreeItem *> m_componentIndicies;
  bool indexIsRegisterOutputPortValue(const QModelIndex &index) const;
};

} // namespace vsrtl

#endif // VSRTL_NETLISTMODEL_H
