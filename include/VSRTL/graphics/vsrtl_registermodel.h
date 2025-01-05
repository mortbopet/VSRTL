#ifndef VSRTL_REGISTERMODEL_H
#define VSRTL_REGISTERMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "vsrtl_netlistmodelbase.h"
#include "vsrtl_register.h"
#include "vsrtl_treeitem.h"

namespace vsrtl {

class RegisterTreeItem : public NetlistTreeItem {
public:
  RegisterTreeItem(TreeItem *parent, SimDesign *design)
      : NetlistTreeItem(parent), m_design(design) {}

  enum class PortDirection { Input, Output };
  QVariant data(int column, int role = Qt::EditRole) const override;
  bool setData(int column, const QVariant &value,
               int role = Qt::EditRole) override;
  QList<QMenu *> getActions() const override;
  void setRegister(SimComponent *reg);

  SimComponent *m_register = nullptr;
  SimDesign *m_design = nullptr;
};

class RegisterModel : public NetlistModelBase<RegisterTreeItem> {
  Q_OBJECT

public:
  enum columns { ComponentColumn, ValueColumn, WidthColumn, NUM_COLUMNS };
  RegisterModel(SimDesign *arch, QObject *parent = nullptr);

  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  bool setData(const QModelIndex &index, const QVariant &value,
               int role = Qt::EditRole) override;

public slots:
  void invalidate() override;

private:
  void loadDesign(RegisterTreeItem *parent, SimDesign *component);

  SimDesign *m_design = nullptr;
};

} // namespace vsrtl

#endif // VSRTL_REGISTERMODEL_H
