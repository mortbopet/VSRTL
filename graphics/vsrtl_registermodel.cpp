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

#include "vsrtl_netlistitem.h"
#include "vsrtl_registermodel.h"

#include "vsrtl_design.h"

namespace vsrtl {

NetlistItem* RegisterModel::getItem(const QModelIndex& index) const {
    if (index.isValid()) {
        NetlistItem* item = static_cast<NetlistItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

bool RegisterModel::indexIsRegisterValue(const QModelIndex& index) const {
    return dynamic_cast<Register*>(getCorePtr<Component*>(index)) != nullptr;
}

RegisterModel::RegisterModel(const Design& arch, QObject* parent) : QAbstractItemModel(parent), m_arch(arch) {
    QStringList headers{"Component", "Value", "Width"};
    QVector<QVariant> rootData;
    for (QString header : headers)
        rootData << header;

    rootItem = new NetlistItem(rootData);

    loadDesign(rootItem, &m_arch);
}

RegisterModel::~RegisterModel() {
    delete rootItem;
}

int RegisterModel::columnCount(const QModelIndex& /* parent */) const {
    return rootItem->columnCount();
}

QVariant RegisterModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    NetlistItem* item = getItem(index);

    // Formatting of "Value" column when a leaf (Register value) is seen
    if (index.column() == 1 && indexIsRegisterValue(index)) {
        switch (role) {
            case Qt::FontRole: {
                return QFont("monospace");
            }
            case Qt::ForegroundRole: {
                return QBrush(Qt::blue);
            }
            case Qt::DisplayRole: {
                VSRTL_VT_U value = item->data(index.column(), role).value<VSRTL_VT_U>();
                return "0x" + QString::number(value, 16).rightJustified(8, '0');
            }
        }
    }

    return item->data(index.column(), role);
}

Qt::ItemFlags RegisterModel::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return 0;
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    // Register values are editable
    if (index.column() == 1 && indexIsRegisterValue(index)) {
        flags |= Qt::ItemIsEditable;
    }

    return flags;
}

QVariant RegisterModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex RegisterModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    NetlistItem* parentItem = getItem(parent);
    if (!parentItem)
        parentItem = rootItem;

    NetlistItem* childItem = parentItem->child(row);
    if (childItem) {
        auto i = createIndex(row, column, childItem);
        childItem->index = i;
        return i;
    } else
        return QModelIndex();
}

bool RegisterModel::insertColumns(int position, int columns, const QModelIndex& parent) {
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool RegisterModel::insertRows(int position, int rows, const QModelIndex& parent) {
    NetlistItem* parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}

QModelIndex RegisterModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();

    NetlistItem* childItem = getItem(index);
    NetlistItem* parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

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
    NetlistItem* parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

int RegisterModel::rowCount(const QModelIndex& parent) const {
    NetlistItem* parentItem = getItem(parent);
    return parentItem->childCount();
}

bool RegisterModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (index.column() == 1) {
        Register* reg = dynamic_cast<Register*>(getCorePtr<Component*>(index));
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

void RegisterModel::updateNetlistItem(NetlistItem* index) {
    auto reg = dynamic_cast<Register*>(getCorePtr<Component*>(index));
    if (reg) {
        index->setData(1, QVariant::fromValue(static_cast<VSRTL_VT_U>(reg->out.value<VSRTL_VT_U>())));
    }
}

void RegisterModel::updateNetlistDataRecursive(NetlistItem* index) {
    updateNetlistItem(index);
    for (int i = 0; i < index->childCount(); i++) {
        updateNetlistDataRecursive(index->child(i));
    }
}

void RegisterModel::updateNetlistData() {
    // For now, just reload the entire model
    updateNetlistDataRecursive(rootItem);
    dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

void RegisterModel::addPortsToComponent(Port* port, NetlistItem* parent, NetlistData::IOType dir) {
    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());

    // Set component data (component name and signal value)S
    NetlistItem* child = parent->child(parent->childCount() - 1);

    child->setData(0, QString::fromStdString(port->getName()));
    child->setData(0, QVariant::fromValue(dir), NetlistRoles::PortType);
    child->setData(0, QVariant::fromValue(port), NetlistRoles::CorePtr);
}

void RegisterModel::loadDesign(NetlistItem* parent, const Design* design) {
    const auto& registers = design->getRegisters();

    std::map<const Component*, NetlistItem*> parentMap;

    const Component* rootComponent = dynamic_cast<const Component*>(design);
    parentMap[rootComponent] = parent;

    // Build a tree representing the hierarchy of components and subcomponents containing registers
    for (const auto& reg : registers) {
        const Component* regParent = reg->getParent();
        NetlistItem* regParentNetlistItem = nullptr;

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
                regParentNetlistItem->insertChildren(regParentNetlistItem->childCount(), 1, rootItem->columnCount());
                regParentNetlistItem = regParentNetlistItem->child(regParentNetlistItem->childCount() - 1);
                regParentNetlistItem->setData(0, QString::fromStdString(p->getName()));
                Q_ASSERT(parentMap.count(p) == 0);
                parentMap[p] = regParentNetlistItem;
            }
            // After the newParentsInTree stack has been iterated through, 'regParentNetlistItem' will point to the
            // parent tree item of the current 'reg' in the outer foor loop
        } else {
            regParentNetlistItem = parentMap[regParent];
        }

        // Add register to its parent tree item
        regParentNetlistItem->insertChildren(regParentNetlistItem->childCount(), 1, rootItem->columnCount());

        // Set component data (component name and signal value)
        NetlistItem* child = regParentNetlistItem->child(regParentNetlistItem->childCount() - 1);
        child->setData(0, QVariant::fromValue(static_cast<Component*>(reg)), NetlistRoles::CorePtr);
        child->setData(0, QString::fromStdString(reg->getName()));
        child->setData(2, QString::number(reg->out.getWidth()));
    }
}

}  // namespace vsrtl
