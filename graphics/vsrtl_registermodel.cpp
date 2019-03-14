/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QItemSelectionModel>
#include <QWidget>

#include <QDebug>

#include "vsrtl_registermodel.h"
#include "vsrtl_treeitem.h"

#include "vsrtl_design.h"

namespace vsrtl {

TreeItem* RegisterModel::getItem(const QModelIndex& index) const {
    if (index.isValid()) {
        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

namespace {
int getRootIndex(QModelIndex index) {
    if (index.isValid()) {
        while (index.parent().isValid()) {
            index = index.parent();
        }
        return index.row();
    } else {
        return -1;
    }
}

int getRootSelectedIndex(QItemSelectionModel* model) {
    auto indexes = model->selectedIndexes();
    if (!indexes.isEmpty()) {
        return getRootIndex(indexes.first());
    } else {
        return -1;
    }
}
}  // namespace

bool RegisterModel::indexIsRegisterOutputPortValue(const QModelIndex& index) const {
    if (index.column() == VALUE_COL) {
        TreeItem* item = getItem(index);
        TreeItem* parentItem = item->parent();
        if (parentItem && parentItem->getUserData().component) {
            if (dynamic_cast<Register*>(parentItem->getUserData().component)) {
                // Parent is register...
                if (item->getUserData().port && item->getUserData().t == NetlistData::IOType::output) {
                    return true;
                }
            }
        }
    }
    return false;
}

Port* RegisterModel::getPort(const QModelIndex& index) const {
    TreeItem* item = getItem(index);
    if (item->getUserData().port) {
        return item->getUserData().port;
    }
    return nullptr;
}

Component* RegisterModel::getComponent(const QModelIndex& index) const {
    TreeItem* item = getItem(index);
    if (item->getUserData().component) {
        return item->getUserData().component;
    }
    return nullptr;
}

Component* RegisterModel::getParentComponent(const QModelIndex& index) const {
    TreeItem* item = getItem(index);
    if (item) {
        item = item->parent();
        if (item) {
            if (item->getUserData().component) {
                return item->getUserData().component;
            }
        }
    }
    return nullptr;
}

RegisterModel::RegisterModel(const Design& arch, QObject* parent) : QAbstractItemModel(parent), m_arch(arch) {
    QStringList headers{"Component", "Value", "Width"};
    QVector<QVariant> rootData;
    for (QString header : headers)
        rootData << header;

    rootItem = new TreeItem(rootData);

    loadDesign(rootItem, &m_arch);
}

//! [0]

//! [1]
RegisterModel::~RegisterModel() {
    delete rootItem;
}
//! [1]

//! [2]
int RegisterModel::columnCount(const QModelIndex& /* parent */) const {
    return rootItem->columnCount();
}
//! [2]

QVariant RegisterModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    TreeItem* item = getItem(index);
    return item->data(index.column(), role);
}

//! [3]
Qt::ItemFlags RegisterModel::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return 0;
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    // Output ports of register components are editable.
    // Check if parent component is a Register, and if the current port is an output port. If so, the port is editable
    if (indexIsRegisterOutputPortValue(index)) {
        flags |= Qt::ItemIsEditable;
    }

    QVariant component = data(index, NetlistRoles::ComponentPtr);
    if (component.isValid()) {
    }
    return flags;
}

QVariant RegisterModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

//! [5]
QModelIndex RegisterModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();
    //! [5]

    //! [6]
    TreeItem* parentItem = getItem(parent);
    if (!parentItem)
        parentItem = rootItem;

    TreeItem* childItem = parentItem->child(row);
    if (childItem) {
        auto i = createIndex(row, column, childItem);
        childItem->index = i;
        return i;
    } else
        return QModelIndex();
}
//! [6]

bool RegisterModel::insertColumns(int position, int columns, const QModelIndex& parent) {
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool RegisterModel::insertRows(int position, int rows, const QModelIndex& parent) {
    TreeItem* parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}

//! [7]
QModelIndex RegisterModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();

    TreeItem* childItem = getItem(index);
    TreeItem* parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}
//! [7]

bool RegisterModel::removeColumns(int position, int columns, const QModelIndex& parent) {
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool RegisterModel::removeRows(int position, int rows, const QModelIndex& parent) {
    TreeItem* parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

//! [8]
int RegisterModel::rowCount(const QModelIndex& parent) const {
    TreeItem* parentItem = getItem(parent);
    return parentItem->childCount();
}
//! [8]

bool RegisterModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (indexIsRegisterOutputPortValue(index)) {
        Register* reg = dynamic_cast<Register*>(getParentComponent(index));
        if (reg) {
            reg->forceValue(value.toInt());
            m_arch.propagateDesign();
            updateNetlistData();
            return true;
        }
    }
    return false;
}

bool RegisterModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role) {
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

void RegisterModel::updateTreeItem(TreeItem* index) {
    const auto itemData = index->data(0, NetlistRoles::ComponentPtr);
    if (!itemData.isNull()) {
        Register* reg = static_cast<Register*>(itemData.value<Component*>());
        index->setData(1, QVariant::fromValue(static_cast<VSRTL_VT_U>(reg->out.value<VSRTL_VT_U>())));
    }
}

void RegisterModel::updateNetlistDataRecursive(TreeItem* index) {
    updateTreeItem(index);
    for (int i = 0; i < index->childCount(); i++) {
        updateNetlistDataRecursive(index->child(i));
    }
}

QModelIndex RegisterModel::lookupIndexForComponent(Component* c) const {
    if (m_componentIndicies.find(c) != m_componentIndicies.end()) {
        TreeItem* item = m_componentIndicies.at(c);
        if (item->index.isValid()) {
            return item->index;
        }
    }
    return QModelIndex();
}

void RegisterModel::updateNetlistData() {
    // For now, just reload the entire model
    updateNetlistDataRecursive(rootItem);
    dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

void RegisterModel::addPortsToComponent(Port* port, TreeItem* parent, NetlistData::IOType dir) {
    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());

    // Set component data (component name and signal value)S
    TreeItem* child = parent->child(parent->childCount() - 1);

    child->setData(0, QString::fromStdString(port->getName()));
    child->setData(0, QVariant::fromValue(dir), NetlistRoles::PortType);
    child->setData(0, QVariant::fromValue(port), NetlistRoles::PortPtr);
}

void RegisterModel::loadDesign(TreeItem* parent, const Design* design) {
    const auto& registers = design->getRegisters();

    std::map<const Component*, TreeItem*> parentMap;

    const Component* rootComponent = dynamic_cast<const Component*>(design);
    parentMap[rootComponent] = parent;

    // Build a tree representing the hierarchy of components and subcomponents containing registers
    for (const auto& reg : registers) {
        const Component* regParent = reg->getParent();
        TreeItem* regParentTreeItem = nullptr;

        if (parentMap.count(regParent) == 0) {
            // Create new parents in the tree until either the root component is detected, or a parent of a parent
            // is already in the tree
            std::vector<const Component*> newParentsInTree;
            while (regParent != rootComponent && parentMap.count(regParent) == 0) {
                newParentsInTree.insert(newParentsInTree.begin(), regParent);
                regParent = regParent->getParent();
            }
            // At this point, the first value in newParentsInTree has its parent present in the tree. Extend the tree
            // from this index
            regParentTreeItem = parentMap[regParent];
            for (const auto& p : newParentsInTree) {
                regParentTreeItem->insertChildren(regParentTreeItem->childCount(), 1, rootItem->columnCount());
                regParentTreeItem = regParentTreeItem->child(regParentTreeItem->childCount() - 1);
                regParentTreeItem->setData(0, QString::fromStdString(p->getName()));
                Q_ASSERT(parentMap.count(p) == 0);
                parentMap[p] = regParentTreeItem;
            }
            // After the newParentsInTree stack has been iterated through, 'regParentTreeItem' will point to the parent
            // tree item of the current 'reg' in the outer foor loop
        } else {
            regParentTreeItem = parentMap[regParent];
        }

        // Add register to its parent tree item
        regParentTreeItem->insertChildren(regParentTreeItem->childCount(), 1, rootItem->columnCount());

        // Set component data (component name and signal value)
        TreeItem* child = regParentTreeItem->child(regParentTreeItem->childCount() - 1);
        child->setData(0, QVariant::fromValue(static_cast<Component*>(reg)), NetlistRoles::ComponentPtr);
        child->setData(0, QString::fromStdString(reg->getName()));
        child->setData(2, QString::number(reg->out.getWidth()));
    }
}

}  // namespace vsrtl
