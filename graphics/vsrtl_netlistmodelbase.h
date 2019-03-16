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

#include "vsrtl_netlistitem.h"

namespace vsrtl {

class Design;
class Component;
class Port;

int getRootIndex(QModelIndex index);
int getRootSelectedIndex(QItemSelectionModel* model);

class NetlistModelBase : public QAbstractItemModel {
    Q_OBJECT

public:
    NetlistModelBase(const Design& arch, QObject* parent = nullptr);
    ~NetlistModelBase() override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value,
                       int role = Qt::EditRole) override;
public slots:
    void updateNetlistData();

protected:
    template <typename T>
    T getCorePtr(NetlistItem* item) const {
        auto p = item->getUserData().coreptr.value<T>();
        return p ? p : nullptr;
    }

    template <typename T>
    T getCorePtr(const QModelIndex& index) const {
        NetlistItem* item = getItem(index);
        return getCorePtr<T>(item);
    }

    bool indexIsRegisterOutputPortValue(const QModelIndex& index) const;
    Component* getParentComponent(const QModelIndex& index) const;
    NetlistItem* getItem(const QModelIndex&) const;
    void updateNetlistDataRecursive(NetlistItem* index);
    virtual void updateNetlistItem(NetlistItem* index) = 0;
    virtual void loadDesign(NetlistItem* parent, const Component& component) = 0;

    NetlistItem* rootItem = nullptr;

    const Design& m_arch;
};

}  // namespace vsrtl

#endif  // VSRTL_NETLISTMODELBASE_H
