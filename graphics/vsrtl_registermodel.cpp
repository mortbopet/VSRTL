#include <QItemSelectionModel>
#include <QWidget>

#include "vsrtl_displaytype.h"
#include "vsrtl_registermodel.h"
#include "vsrtl_treeitem.h"

#include "vsrtl_design.h"

namespace vsrtl {

QList<QAction*> RegisterTreeItem::getActions() const {
    // Only return actions for items which have a port (default actions from TreeItem displays display type actions,
    // which are not applicable for Component items)
    return m_register != nullptr ? TreeItem::getActions() : QList<QAction*>();
}

QVariant RegisterTreeItem::data(int column, int role) const {
    if (column == 1 && m_register != nullptr) {
        switch (role) {
            case Qt::FontRole: {
                return QFont("monospace");
            }
            case Qt::ForegroundRole: {
                return QBrush(Qt::blue);
            }
            case Qt::DisplayRole: {
                VSRTL_VT_U value = m_register->out.template value<VSRTL_VT_U>();
                return encodeDisplayValue(value, m_register->out.getWidth(), m_displayType);
            }
        }
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
            case RegisterModel::ComponentColumn: {
                return m_name;
            }
            case RegisterModel::WidthColumn: {
                if (m_register) {
                    return m_register->out.getWidth();
                }
                break;
            }
        }
    }

    return QVariant();
}
bool RegisterTreeItem::setData(int column, const QVariant& value, int role) {
    if (index.column() == RegisterModel::ValueColumn) {
        if (m_register) {
            m_register->forceValue(value.value<VSRTL_VT_U>());
            return true;
        }
    }
    return false;
}

RegisterModel::RegisterModel(const Design& arch, QObject* parent)
    : NetlistModelBase({"Component", "Value", "Width"}, arch, parent) {
    rootItem = new RegisterTreeItem(nullptr);
    loadDesign(rootItem, m_arch);
}

QVariant RegisterModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    auto* item = getTreeItem(index);
    return item->data(index.column(), role);
}

Qt::ItemFlags RegisterModel::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return 0;
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    // Register values are editable
    if (index.column() == 1 && getTreeItem(index)->m_register != nullptr) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

bool RegisterModel::setData(const QModelIndex& index, const QVariant& var, int role) {
    auto* item = getTreeItem(index);
    if (item) {
        VSRTL_VT_U value = decodeDisplayValue(var.toString(), item->m_register->out.getWidth(), item->m_displayType);
        bool resval = item->setData(index.column(), value, role);
        if (resval) {
            m_arch.propagateDesign();
        }
        return resval;
    }

    return false;
}

void RegisterModel::loadDesign(RegisterTreeItem* parent, const Design& design) {
    const auto& registers = design.getRegisters();

    std::map<const Component*, RegisterTreeItem*> parentMap;

    const Component* rootComponent = dynamic_cast<const Component*>(&design);
    parentMap[rootComponent] = parent;

    // Build a tree representing the hierarchy of components and subcomponents containing registers
    for (const auto& reg : registers) {
        const Component* regParent = reg->getParent();
        RegisterTreeItem* regParentNetlistItem = nullptr;

        if (parentMap.count(regParent) == 0) {
            // Create new parents in the tree until either the root component is detected, or a parent of a parent
            // is already in the tree
            std::vector<const Component*> newParentsInTree;
            while (regParent != rootComponent && parentMap.count(regParent) == 0) {
                newParentsInTree.insert(newParentsInTree.begin(), regParent);
                regParent = regParent->getParent();
            }
            // At this point, the first value in newParentsInTree has its parent present in the tree. Extend the
            // tree from this index
            regParentNetlistItem = parentMap[regParent];
            for (const auto& p : newParentsInTree) {
                auto* newParent = new RegisterTreeItem(regParentNetlistItem);
                regParentNetlistItem->insertChild(regParentNetlistItem->childCount(), newParent);
                regParentNetlistItem = newParent;
                regParentNetlistItem->m_name = QString::fromStdString(p->getName());
                Q_ASSERT(parentMap.count(p) == 0);
                parentMap[p] = regParentNetlistItem;
            }
            // After the newParentsInTree stack has been iterated through, 'regParentNetlistItem' will point to the
            // parent tree item of the current 'reg' in the outer foor loop
        } else {
            regParentNetlistItem = parentMap[regParent];
        }

        // Add register to its parent tree item

        auto* child = new RegisterTreeItem(regParentNetlistItem);
        regParentNetlistItem->insertChild(regParentNetlistItem->childCount(), child);

        // Set component data (component name and signal value)
        child->m_register = reg;
        child->m_name = QString::fromStdString(reg->getName());
    }
}

}  // namespace vsrtl
