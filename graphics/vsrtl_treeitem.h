#ifndef VSRTL_TREEITEM_H
#define VSRTL_TREEITEM_H

#include <QList>
#include <QModelIndex>
#include <QVariant>
#include <QVector>

#include "vsrtl_displaytype.h"

namespace vsrtl {

/** Generic tree structure node class */
class TreeItem {
public:
    explicit TreeItem(TreeItem* parent);
    virtual ~TreeItem();

    virtual QVariant data(int column, int role = Qt::EditRole) const = 0;
    virtual bool setData(int column, const QVariant& value, int role = Qt::EditRole) = 0;

    TreeItem* child(int number);
    int childCount() const;
    bool insertChild(int position, TreeItem* item);
    TreeItem* parent();
    bool removeChildren(int position, int count);
    int childNumber() const;

    // Store an index to the QModelIndex corresponding to this item in the tree. This is required for facilitating
    // selection behaviour via. selections made in the graphics component.
    QModelIndex index;

    QString m_name;
    DisplayType m_displayType = DisplayType::Hex;
    QList<TreeItem*> childItems;
    TreeItem* parentItem;
};

}  // namespace vsrtl

#endif  // VSRTL_TREEITEM_H
