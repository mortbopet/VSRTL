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

#include "vsrtl_netlistitem.h"

#include <QIcon>
#include <QStringList>

namespace vsrtl {

NetlistItem::NetlistItem(const QVector<QVariant>& data, NetlistItem* parent) {
    parentItem = parent;
    itemData = data;
}

NetlistItem::~NetlistItem() {
    qDeleteAll(childItems);
}

NetlistItem* NetlistItem::child(int number) {
    return childItems.value(number);
}

int NetlistItem::childCount() const {
    return childItems.count();
}

int NetlistItem::childNumber() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<NetlistItem*>(this));

    return 0;
}

int NetlistItem::columnCount() const {
    return itemData.count();
}

QVariant NetlistItem::data(int column, int role) const {
    if (role == Qt::UserRole) {
        return QVariant::fromValue(userData);
    } else if (role == Qt::DecorationRole) {
        if (column == IO_COL) {
            switch (userData.t) {
                case NetlistData::IOType::input:
                    return QIcon(":/icons/input.svg");
                case NetlistData::IOType::output:
                    return QIcon(":/icons/output.svg");
                default:
                    break;
            }
        }
    } else if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return itemData.value(column);
    } else if (role == NetlistRoles::CorePtr) {
        return userData.coreptr;
    }
    return QVariant();
}  // namespace vsrtl

bool NetlistItem::insertChildren(int position, int count, int columns) {
    if (position < 0 || position > childItems.size())
        return false;

    for (int row = 0; row < count; ++row) {
        QVector<QVariant> data(columns);
        NetlistItem* item = new NetlistItem(data, this);
        childItems.insert(position, item);
    }

    return true;
}

bool NetlistItem::insertColumns(int position, int columns) {
    if (position < 0 || position > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.insert(position, QVariant());

    foreach (NetlistItem* child, childItems)
        child->insertColumns(position, columns);

    return true;
}

NetlistItem* NetlistItem::parent() {
    return parentItem;
}

bool NetlistItem::removeChildren(int position, int count) {
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

bool NetlistItem::removeColumns(int position, int columns) {
    if (position < 0 || position + columns > itemData.size())
        return false;

    for (int column = 0; column < columns; ++column)
        itemData.remove(position);

    foreach (NetlistItem* child, childItems)
        child->removeColumns(position, columns);

    return true;
}

bool NetlistItem::setData(int column, const QVariant& value, int role) {
    if (column < 0 || column >= itemData.size())
        return false;

    if (role == Qt::EditRole) {
        itemData[column] = value;
    } else if (role == NetlistRoles::CorePtr) {
        userData.coreptr = value;
    } else if (role == NetlistRoles::PortType) {
        userData.t = value.value<NetlistData::IOType>();
    }

    return true;
}

}  // namespace vsrtl
