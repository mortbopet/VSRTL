#include <QItemSelectionModel>
#include <QWidget>

#include <QDebug>
#include <QIcon>

#include "vsrtl_netlistmodel.h"

#include "core/vsrtl_design.h"

namespace vsrtl {

QList<QMenu*> NetlistTreeItem::getActions() const {
    // Only return actions for items which have a port (default actions from TreeItem displays display type actions,
    // which are not applicable for Component items)
    return m_port != nullptr ? TreeItem::getActions() : QList<QMenu*>();
}

QVariant NetlistTreeItem::data(int column, int role) const {
    if (column == NetlistModel::IOColumn && role == Qt::DecorationRole && m_port != nullptr) {
        return m_direction == PortDirection::Input ? QIcon(":/icons/input.svg") : QIcon(":/icons/output.svg");
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
            case NetlistModel::ComponentColumn: {
                return m_name;
            }
            case NetlistModel::ValueColumn: {
                if (m_port) {
                    VSRTL_VT_U value = m_port->template value<VSRTL_VT_U>();
                    return encodeDisplayValue(value, m_port->getWidth(), m_displayType);
                }
                break;
            }
            case NetlistModel::WidthColumn: {
                if (m_port) {
                    return m_port->getWidth();
                }
                break;
            }
        }
    }
    return QVariant();
}
bool NetlistTreeItem::setData(int column, const QVariant& value, int role) {
    return false;
}

NetlistModel::NetlistModel(Design& arch, QObject* parent)
    : NetlistModelBase({"Component", "I/O", "Value", "Width"}, arch, parent) {
    rootItem = new NetlistTreeItem(nullptr);

    loadDesign(rootItem, m_arch);
}

QVariant NetlistModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    NetlistTreeItem* item = getTreeItem(index);
    return item->data(index.column(), role);
}

Qt::ItemFlags NetlistModel::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return 0;
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    // Output ports of register components are editable.
    // Check if parent component is a Register, and if the current port is an output port. If so, the port is editable
    if (indexIsRegisterOutputPortValue(index)) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

bool NetlistModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (indexIsRegisterOutputPortValue(index)) {
        Register* reg = dynamic_cast<Register*>(getParentComponent(index));
        if (reg) {
            reg->forceValue(value.toInt());
            m_arch.propagateDesign();
            return true;
        }
    }
    return false;
}

void NetlistModel::invalidate() {
    // Data changed within Design, invalidate value column
    dataChanged(index(0, ValueColumn), index(rowCount(), ValueColumn), {Qt::DisplayRole});
}

QModelIndex NetlistModel::lookupIndexForComponent(Component* c) const {
    if (m_componentIndicies.find(c) != m_componentIndicies.end()) {
        NetlistTreeItem* item = m_componentIndicies.at(c);
        if (item->index.isValid()) {
            return item->index;
        }
    }
    return QModelIndex();
}

void NetlistModel::addPortToComponent(Port* port, NetlistTreeItem* parent, PortDirection dir) {
    auto* child = new NetlistTreeItem(parent);
    parent->insertChild(parent->childCount(), child);

    child->m_name = QString::fromStdString(port->getName());
    child->m_direction = dir;
    child->m_port = port;
}

bool NetlistModel::indexIsRegisterOutputPortValue(const QModelIndex& index) const {
    if (index.column() == ValueColumn) {
        auto* item = getTreeItem(index);
        auto* parentItem = static_cast<NetlistTreeItem*>(item->parent());
        auto* parentComponent = parentItem->m_component;
        if (parentItem && parentComponent) {
            if (dynamic_cast<Register*>(parentComponent)) {
                // Parent is register, check if current index is an output port
                if (item->m_port && item->m_direction == PortDirection::Output) {
                    return true;
                }
            }
        }
    }
    return false;
}

void NetlistModel::loadDesignRecursive(NetlistTreeItem* parent, const Component& component) {
    auto& subComponents = component.getSubComponents();

    // Subcomponents
    for (const auto& subcomponent : subComponents) {
        auto* child = new NetlistTreeItem(parent);
        parent->insertChild(parent->childCount(), child);

        // Set component data (component pointer and name)
        m_componentIndicies[subcomponent.get()] = child;

        child->m_component = subcomponent.get();
        child->m_name = QString::fromStdString(subcomponent->getName());

        // Recurse into the child
        loadDesignRecursive(child, *subcomponent);
    }

    // I/O ports of component
    for (const auto& input : component.getInputs()) {
        addPortToComponent(input.get(), parent, PortDirection::Input);
    }
    for (const auto& output : component.getOutputs()) {
        addPortToComponent(output.get(), parent, PortDirection::Output);
    }
}

Component* NetlistModel::getParentComponent(const QModelIndex& index) const {
    auto* item = getTreeItem(index);
    if (item) {
        item = static_cast<NetlistTreeItem*>(item->parent());
        if (item) {
            return item->m_component;
        }
    }
    return nullptr;
}

void NetlistModel::loadDesign(NetlistTreeItem* parent, const Design& design) {
    loadDesignRecursive(parent, design);
}
}  // namespace vsrtl
