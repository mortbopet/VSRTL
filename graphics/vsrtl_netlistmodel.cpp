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

#include "vsrtl_netlistmodel.h"
#include "vsrtl_treeitem.h"

#include "vsrtl_architecture.h"

namespace vsrtl {

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

//! [0]
NetlistModel::NetlistModel(const Architecture& arch, QObject* parent) : QAbstractItemModel(parent), m_arch(arch) {
    QStringList headers{"Component", "Value"};
    QVector<QVariant> rootData;
    for (QString header : headers)
        rootData << header;

    rootItem = new TreeItem(rootData);

    reloadNetlistData(rootItem, arch);
}
//! [0]

//! [1]
NetlistModel::~NetlistModel() {
    delete rootItem;
}
//! [1]

//! [2]
int NetlistModel::columnCount(const QModelIndex& /* parent */) const {
    return rootItem->columnCount();
}
//! [2]

QVariant NetlistModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole && role != Qt::UserRole) {
        return QVariant();
    }

    TreeItem* item = getItem(index);
    return item->data(index.column(), role);
}

//! [3]
Qt::ItemFlags NetlistModel::flags(const QModelIndex&) const {
    return Qt::NoItemFlags;
}
//! [3]

//! [4]
TreeItem* NetlistModel::getItem(const QModelIndex& index) const {
    if (index.isValid()) {
        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}
//! [4]

QVariant NetlistModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

//! [5]
QModelIndex NetlistModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();
    //! [5]

    //! [6]
    TreeItem* parentItem = getItem(parent);

    TreeItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}
//! [6]

bool NetlistModel::insertColumns(int position, int columns, const QModelIndex& parent) {
    bool success;

    beginInsertColumns(parent, position, position + columns - 1);
    success = rootItem->insertColumns(position, columns);
    endInsertColumns();

    return success;
}

bool NetlistModel::insertRows(int position, int rows, const QModelIndex& parent) {
    TreeItem* parentItem = getItem(parent);
    bool success;

    beginInsertRows(parent, position, position + rows - 1);
    success = parentItem->insertChildren(position, rows, rootItem->columnCount());
    endInsertRows();

    return success;
}

//! [7]
QModelIndex NetlistModel::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();

    TreeItem* childItem = getItem(index);
    TreeItem* parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}
//! [7]

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
    TreeItem* parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    success = parentItem->removeChildren(position, rows);
    endRemoveRows();

    return success;
}

//! [8]
int NetlistModel::rowCount(const QModelIndex& parent) const {
    TreeItem* parentItem = getItem(parent);

    return parentItem->childCount();
}
//! [8]

bool NetlistModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role == Qt::EditRole) {
        if (m_modelIsLoading) {
            // We are loading data from the processInterface into the model
            TreeItem* item = getItem(index);
            bool result = item->setData(index.column(), value, role);

            if (result)
                emit dataChanged(index, index);

            return result;
        } else {
            return true;
        }
    } else if (role == Qt::ToolTipRole || role == Qt::UserRole) {
        TreeItem* item = getItem(index);
        return item->setData(index.column(), value, role);
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

void NetlistModel::reloadNetlistData(TreeItem* parent, const Component& component) {
    auto& subComponents = component.getSubComponents();

    // Subcomponents
    for (const auto& subcomponent : subComponents) {
        parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());

        // Set component data (component name and signal value)
        TreeItem* child = parent->child(parent->childCount() - 1);
        child->setData(0, QString::fromStdString(subcomponent->getDisplayName()));

        // Recurse into the child
        reloadNetlistData(child, *subcomponent);
    }

    // I/O ports of component
    for (const auto& input : component.getInputs()) {
        parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());

        // Set component data (component name and signal value)S
        TreeItem* child = parent->child(parent->childCount() - 1);

        child->setData(0, QString::fromStdString((*(*(*(input)))).getName()));
        child->setData(1, static_cast<unsigned int>(*(*(*(input)))));
    }
    for (const auto& output : component.getOutputs()) {
        parent->insertChildren(parent->childCount(), 1, rootItem->columnCount());

        // Set component data (component name and signal value)S
        TreeItem* child = parent->child(parent->childCount() - 1);

        child->setData(0, QString::fromStdString((*(output)).getName()));
        child->setData(1, static_cast<unsigned int>(*(output)));
    }
}

}  // namespace vsrtl
