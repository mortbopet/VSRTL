#ifndef VSRTL_NETLISTMODEL_H
#define VSRTL_NETLISTMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

#include "vsrtl_netlistmodelbase.h"
#include "vsrtl_treeitem.h"

namespace vsrtl {

class Design;
class Component;
class Port;

enum class PortDirection { Input, Output };

class NetlistTreeItem : public TreeItem {
public:
    NetlistTreeItem(TreeItem* parent) : TreeItem(parent) {}
    QVariant data(int column, int role = Qt::EditRole) const override;
    bool setData(int column, const QVariant& value, int role = Qt::EditRole) override;
    QList<QMenu*> getActions() const override;

    Component* m_component = nullptr;
    Port* m_port = nullptr;
    PortDirection m_direction;
};

class NetlistModel : public NetlistModelBase<NetlistTreeItem> {
    Q_OBJECT

public:
    enum columns { ComponentColumn, IOColumn, ValueColumn, WidthColumn, NUM_COLUMNS };
    NetlistModel(Design& arch, QObject* parent = nullptr);

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    QModelIndex lookupIndexForComponent(Component* c) const;

public slots:
    void invalidate() override;

private:
    void addPortToComponent(Port* port, NetlistTreeItem* parent, PortDirection);
    void loadDesign(NetlistTreeItem* parent, const Design& design);
    void loadDesignRecursive(NetlistTreeItem* parent, const Component& component);
    Component* getParentComponent(const QModelIndex& index) const;
    std::map<Component*, NetlistTreeItem*> m_componentIndicies;
    bool indexIsRegisterOutputPortValue(const QModelIndex& index) const;
};

}  // namespace vsrtl

#endif  // VSRTL_NETLISTMODEL_H
