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

#ifndef VSRTL_NETLISTMODELBASE_H
#define VSRTL_NETLISTMODELBASE_H

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QVariant>

namespace vsrtl {

class Design;
class Component;
class Port;

int getRootIndex(QModelIndex index);
int getRootSelectedIndex(QItemSelectionModel* model);

template <typename T>
class NetlistModelBase : public QAbstractItemModel {
public:
    NetlistModelBase(QStringList headers, const Design& arch, QObject* parent = nullptr)
        : m_headers(headers), m_arch(arch), QAbstractItemModel(parent) {}

    ~NetlistModelBase() override { delete rootItem; }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole && m_headers.size() > section)
            return m_headers[section];

        return QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override {
        if (parent.isValid() && parent.column() != 0)
            return QModelIndex();

        T* parentItem = getItem<T*>(parent);
        if (!parentItem)
            parentItem = rootItem;

        T* childItem = static_cast<T*>(parentItem->child(row));
        if (childItem) {
            auto i = createIndex(row, column, childItem);
            childItem->index = i;
            return i;
        } else
            return QModelIndex();
    }

    QModelIndex parent(const QModelIndex& index) const override {
        if (!index.isValid())
            return QModelIndex();

        auto* childItem = getItem<T*>(index);
        auto* parentItem = childItem->parent();

        if (parentItem == rootItem)
            return QModelIndex();

        return createIndex(parentItem->childNumber(), 0, parentItem);
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        auto* parentItem = getItem<T*>(parent);
        return parentItem->childCount();
    }

    int columnCount(const QModelIndex& = QModelIndex()) const override { return m_headers.size(); }

    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
                       int role = Qt::EditRole) override {
        if (role != Qt::EditRole || orientation != Qt::Horizontal || section > m_headers.size())
            return false;
        m_headers[section] = value.toString();

        emit headerDataChanged(orientation, section, section);
        return true;
    }
public slots:
    void invalidate() { dataChanged(index(0, 0), index(rowCount(), columnCount())); }

protected:
    template <typename UDT>  // Userdata type
    T* getItem(const QModelIndex& index) const {
        if (index.isValid()) {
            return static_cast<T*>(index.internalPointer());
        }
        return rootItem;
    }

    T* rootItem = nullptr;
    const Design& m_arch;
    QStringList m_headers;
};

}  // namespace vsrtl

#endif  // VSRTL_NETLISTMODELBASE_H
