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

#include "vsrtl_netlistmodel.h"
#include "vsrtl_netlistitem.h"

#include "vsrtl_design.h"

namespace vsrtl {

NetlistItem* NetlistModel::getItem(const QModelIndex& index) const {
    if (index.isValid()) {
        NetlistItem* item = static_cast<NetlistItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

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

bool NetlistModel::indexIsRegisterOutputPortValue(const QModelIndex& index) const {
    if (index.column() == VALUE_COL) {
        NetlistItem* item = getItem(index);
        NetlistItem* parentItem = item->parent();
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

Port* NetlistModel::getPort(const QModelIndex& index) const {
    NetlistItem* item = getItem(index);
    if (item->getUserData().port) {
        return item->getUserData().port;
    }
    return nullptr;
}

Component* NetlistModel::getComponent(const QModelIndex& index) const {
    NetlistItem* item = getItem(index);
    if (item->getUserData().component) {
        return item->getUserData().component;
    }
    return nullptr;
}

Component* NetlistModel::getParentComponent(const QModelIndex& index) const {
    NetlistItem* item = getItem(index);
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

NetlistModel::NetlistModel(const Design& arch, QObject* parent) : QAbstractItemModel(parent), m_arch(arch) {
    QStringList headers{"Component", "I/O", "Value"};
    QVector<QVariant> rootData;
    for (QString header : headers)
        rootData << header;

    rootItem = new NetlistItem(rootData);

    loadDesign(rootItem, m_arch);
}




NetlistModel::~NetlistModel() {
    delete rootItem;
}



int NetlistModel::columnCount(const QModelIndex& /* parent */) const {
    return rootItem->columnCount();
}


QVariant NetlistModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    NetlistItem* item = getItem(index);
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

    QVariant component = data(index, NetlistRoles::ComponentPtr);
    if (component.isValid()) {
    }
    return flags;
}

QVariant NetlistModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}


QModelIndex NetlistModel::index(int row, int column, const QModelIndex& parent) const {
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


bool NetlistModel::insertColumns(int position, int columns, const QModelIndex& parent) {
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool NetlistModel::insertRows(int position, int rows, const QModelIndex& parent) {
    NetlistItem* parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}


QModelIndex NetlistModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();

    NetlistItem* childItem = getItem(index);
    NetlistItem* parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}


bool NetlistModel::removeColumns(int position, int columns, const QModelIndex& parent) {
    bool success;

    beginRemoveColumns(parent, position, position + columns - 1);
    success = rootItem->removeColumns(position, columns);
    endRemoveColumns();

    if (rootItem->columnCount() == 0)
        removeRows(0, rowCount());

    return success;
}

bool NetlistModel::removeRows(int position, int rows, const QModelIndex& parent) {
    NetlistItem* parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}


int NetlistModel::rowCount(const QModelIndex& parent) const {
    NetlistItem* parentItem = getItem(parent);
    return parentItem->childCount();
}


bool NetlistModel::setData(const QModelIndex& index, const QVariant& value, int role) {
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

bool NetlistModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role) {
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

void NetlistModel::updateNetlistItem(NetlistItem* index) {
    const auto itemData = index->data(0, NetlistRoles::PortPtr);
    if (!itemData.isNull()) {
        index->setData(2, QVariant::fromValue(static_cast<VSRTL_VT_U>(*(itemData.value<Port*>()))));
    }
}

void NetlistModel::updateNetlistDataRecursive(NetlistItem* index) {
    updateNetlistItem(index);
    for (int i = 0; i < index->childCount(); i++) {
        updateNetlistDataRecursive(index->child(i));
    }
}

QModelIndex NetlistModel::lookupIndexForComponent(Component* c) const {
    if (m_componentIndicies.find(c) != m_componentIndicies.end()) {
        NetlistItem* item = m_componentIndicies.at(c);
        if (item->index.isValid()) {
            return item->index;
        }
    }
    return QModelIndex();
}

void NetlistModel::updateNetlistData() {
    // For now, just reload the entire model
    updateNetlistDataRecursive(rootItem);
    dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

void NetlistModel::addPortsToComponent(Port* port, NetlistItem* parent, NetlistData::IOType dir) {
    parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());

    // Set component data (component name and signal value)S
    NetlistItem* child = parent->child(parent->childCount() - 1);

    child->setData(0, QString::fromStdString(port->getName()));
    child->setData(0, QVariant::fromValue(dir), NetlistRoles::PortType);
    child->setData(0, QVariant::fromValue(port), NetlistRoles::PortPtr);
}

void NetlistModel::loadDesign(NetlistItem* parent, const Component& component) {
    auto& subComponents = component.getSubComponents();

    // Subcomponents
    for (const auto& subcomponent : subComponents) {
        parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());

        // Set component data (component name and signal value)
        NetlistItem* child = parent->child(parent->childCount() - 1);
        m_componentIndicies[subcomponent.get()] = child;
        child->setData(0, QVariant::fromValue(subcomponent.get()), NetlistRoles::ComponentPtr);
        child->setData(0, QString::fromStdString(subcomponent->getName()));

        // Recurse into the child
        loadDesign(child, *subcomponent);
    }

    // I/O ports of component
    for (const auto& input : component.getInputs()) {
        addPortsToComponent(input.get(), parent, NetlistData::IOType::input);
    }
    for (const auto& output : component.getOutputs()) {
        addPortsToComponent(output.get(), parent, NetlistData::IOType::output);
    }
}

}  // namespace vsrtl
