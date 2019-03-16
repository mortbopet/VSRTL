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

#include <QWidget>

#include <QDebug>

#include "vsrtl_netlistitem.h"
#include "vsrtl_netlistmodelbase.h"

#include "vsrtl_design.h"

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

NetlistItem* NetlistModelBase::getItem(const QModelIndex& index) const {
    if (index.isValid()) {
        NetlistItem* item = static_cast<NetlistItem*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

bool NetlistModelBase::indexIsRegisterOutputPortValue(const QModelIndex& index) const {
    if (index.column() == VALUE_COL) {
        NetlistItem* item = getItem(index);
        NetlistItem* parentItem = item->parent();
        Component* parentComponent = parentItem->getUserData().coreptr.value<Component*>();
        if (parentItem && parentComponent) {
            if (dynamic_cast<Register*>(parentComponent)) {
                // Parent is register...
                if (getCorePtr<Port*>(index) && item->getUserData().t == NetlistData::IOType::output) {
                    return true;
                }
            }
        }
    }
    return false;
}

Component* NetlistModelBase::getParentComponent(const QModelIndex& index) const {
    NetlistItem* item = getItem(index);
    if (item) {
        item = item->parent();
        if (item) {
            return getCorePtr<Component*>(item->index);
        }
    }
    return nullptr;
}

NetlistModelBase::NetlistModelBase(const Design& arch, QObject* parent) : QAbstractItemModel(parent), m_arch(arch) {}

NetlistModelBase::~NetlistModelBase() {
    delete rootItem;
}

QVariant NetlistModelBase::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex NetlistModelBase::index(int row, int column, const QModelIndex& parent) const {
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

QModelIndex NetlistModelBase::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();

    NetlistItem* childItem = getItem(index);
    NetlistItem* parentItem = childItem->parent();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int NetlistModelBase::rowCount(const QModelIndex& parent) const {
    NetlistItem* parentItem = getItem(parent);
    return parentItem->childCount();
}

int NetlistModelBase::columnCount(const QModelIndex& /* parent */) const {
    return rootItem->columnCount();
}

bool NetlistModelBase::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role) {
    if (role != Qt::EditRole || orientation != Qt::Horizontal)
        return false;

    bool result = rootItem->setData(section, value);

    if (result)
        emit headerDataChanged(orientation, section, section);

    return result;
}

void NetlistModelBase::updateNetlistDataRecursive(NetlistItem* index) {
    updateNetlistItem(index);
    for (int i = 0; i < index->childCount(); i++) {
        updateNetlistDataRecursive(index->child(i));
    }
}

void NetlistModelBase::updateNetlistData() {
    // For now, just reload the entire model
    updateNetlistDataRecursive(rootItem);
    dataChanged(index(0, 0), index(rowCount(), columnCount()));
}

}  // namespace vsrtl
