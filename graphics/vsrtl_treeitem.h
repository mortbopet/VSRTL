#ifndef VSRTL_TREEITEM_H
#define VSRTL_TREEITEM_H

#include <QAction>
#include <QList>
#include <QModelIndex>
#include <QVariant>
#include <QVector>

#include "vsrtl_radix.h"

QT_FORWARD_DECLARE_CLASS(QMenu)

namespace vsrtl {

/** Generic tree structure node class */
class TreeItem : public QObject {
    Q_OBJECT
public:
    explicit TreeItem(TreeItem* parent);
    virtual ~TreeItem();

    virtual QVariant data(int column, int role = Qt::EditRole) const = 0;
    virtual bool setData(int column, const QVariant& value, int role = Qt::EditRole) = 0;
    virtual QList<QMenu*> getActions() const;

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
    Radix m_Radix = Radix::Hex;
    QList<TreeItem*> childItems;
    TreeItem* parentItem;

private:
    QMenu* m_RadixMenu;
};

}  // namespace vsrtl

#endif  // VSRTL_TREEITEM_H
