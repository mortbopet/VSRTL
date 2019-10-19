#include "vsrtl_treeitem.h"

#include <QMenu>

namespace vsrtl {

TreeItem::TreeItem(TreeItem* parent) {
    parentItem = parent;

    // Display type actions
    m_displayTypeMenu = createDisplayTypeMenu(m_displayType);
}

TreeItem::~TreeItem() {
    qDeleteAll(childItems);
}

TreeItem* TreeItem::child(int number) {
    return childItems.value(number);
}

int TreeItem::childCount() const {
    return childItems.count();
}

int TreeItem::childNumber() const {
    if (parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

    return 0;
}

QList<QMenu*> TreeItem::getActions() const {
    return {m_displayTypeMenu};
}

bool TreeItem::insertChild(int position, TreeItem* item) {
    if (position < 0 || position > childItems.size())
        return false;

    childItems.insert(position, item);

    return true;
}

TreeItem* TreeItem::parent() {
    return parentItem;
}

bool TreeItem::removeChildren(int position, int count) {
    if (position < 0 || position + count > childItems.size())
        return false;

    for (int row = 0; row < count; ++row)
        delete childItems.takeAt(position);

    return true;
}

}  // namespace vsrtl
