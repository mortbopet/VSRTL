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

#ifndef VSRTL_TREEITEM_H
#define VSRTL_TREEITEM_H

#include <QList>
#include <QVariant>
#include <QVector>

#include "vsrtl_signal.h"

namespace vsrtl {

// TreeItem's are structured as having, in column 0; the NetlistData as userData
// wherein the value is read and assigned to column 1

typedef struct {
    enum class type { invalid, input, output };
    type t = type::invalid;
    InputSignalBase* input = nullptr;
    OutputSignalBase* output = nullptr;
} NetlistData;

//! [0]
class TreeItem {
public:
    explicit TreeItem(const QVector<QVariant>& data, TreeItem* parent = 0);
    ~TreeItem();

    TreeItem* child(int number);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column, int role = Qt::EditRole) const;
    bool insertChildren(int position, int count, int columns);
    bool insertColumns(int position, int columns);
    TreeItem* parent();
    bool removeChildren(int position, int count);
    bool removeColumns(int position, int columns);
    int childNumber() const;
    bool setData(int column, const QVariant& value, int role = Qt::EditRole);

private:
    QList<TreeItem*> childItems;
    QVector<QVariant> itemData;
    QString tooltip;
    TreeItem* parentItem;
    NetlistData userData;
};
//! [0]

}  // namespace vsrtl

// Declare as qMetatype to be able to use as a qVariant
Q_DECLARE_METATYPE(vsrtl::NetlistData)

#endif  // VSRTL_TREEITEM_H
