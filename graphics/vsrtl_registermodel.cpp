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
#include "vsrtl_treeitem.h"

#include "vsrtl_design.h"

namespace vsrtl {

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
                return "0x" + QString::number(value, 16).rightJustified(8, '0');
            }
        }
    }

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
            case RegisterModel::ComponentColumn: {
                return getName();
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
            m_register->forceValue(value.toInt());
            return true;
        }
    }
    return false;
}

bool RegisterModel::indexIsRegisterValue(const QModelIndex& index) const {
    return getItem<RegisterTreeItem*>(index)->m_register != nullptr;
}

RegisterModel::RegisterModel(const Design& arch, QObject* parent)
    : NetlistModelBase({"Component", "Value", "Width"}, arch, parent) {
    rootItem = new RegisterTreeItem(nullptr);
    loadDesign(rootItem, m_arch);
}

QVariant RegisterModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid())
        return QVariant();

    auto* item = getItem<RegisterTreeItem*>(index);

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

bool RegisterModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    auto* item = getItem<RegisterTreeItem*>(index);
    if (item) {
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
                regParentNetlistItem->setName(QString::fromStdString(p->getName()));
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
        child->setName(QString::fromStdString(reg->getName()));
    }
}

}  // namespace vsrtl
