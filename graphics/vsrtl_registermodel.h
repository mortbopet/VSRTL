#ifndef VSRTL_REGISTERMODEL_H
#define VSRTL_REGISTERMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "vsrtl_netlistitem.h"
#include "vsrtl_netlistmodelbase.h"
#include "vsrtl_treeitem.h"

namespace vsrtl {

class Register;

class RegisterTreeItem : public TreeItem {
public:
    RegisterTreeItem(TreeItem* parent) : TreeItem(parent) {}

    enum class PortDirection { Input, Output };
    QVariant data(int column, int role = Qt::EditRole) const override;
    bool setData(int column, const QVariant& value, int role = Qt::EditRole) override;

    Register* m_register = nullptr;
};

class RegisterModel : public NetlistModelBase<RegisterTreeItem> {
    Q_OBJECT

public:
    enum columns { ComponentColumn, ValueColumn, WidthColumn, NUM_COLUMNS };
    RegisterModel(const Design& arch, QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

private:
    void loadDesign(RegisterTreeItem* parent, const Design& component);
    bool indexIsRegisterValue(const QModelIndex& index) const;
};

}  // namespace vsrtl

#endif  // VSRTL_REGISTERMODEL_H
